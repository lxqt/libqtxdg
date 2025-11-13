/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "xdgmenu.h"
#include "xdgmenu_p.h"
#include "xdgmenureader.h"
#include "xmlhelper.h"
#include "xdgmenurules.h"
#include "xdgmenuapplinkprocessor.h"
#include "xdgdirs.h"
#include "xdgmenulayoutprocessor.h"

#include <QDebug>
#include <QtXml/QDomElement>
#include <QtXml/QDomNamedNodeMap>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QSettings>
#include <QDir>
#include <QHash>
#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QCryptographicHash>

using namespace Qt::Literals::StringLiterals;

// Helper functions prototypes
void installTranslation(const QString &name);
bool isParent(const QDomElement& parent, const QDomElement& child);


XdgMenu::XdgMenu(QObject *parent) :
    QObject(parent),
    d_ptr(new XdgMenuPrivate(this))
{
}


XdgMenu::~XdgMenu()
{
    Q_D(XdgMenu);
    delete d;
}


XdgMenuPrivate::XdgMenuPrivate(XdgMenu *parent):
    mOutDated(true),
    q_ptr(parent)
{
    mRebuildDelayTimer.setSingleShot(true);
    mRebuildDelayTimer.setInterval(REBUILD_DELAY);

    connect(&mRebuildDelayTimer, &QTimer::timeout, this, &XdgMenuPrivate::rebuild);
    connect(&mWatcher, &QFileSystemWatcher::fileChanged, &mRebuildDelayTimer, QOverload<>::of(&QTimer::start));
    connect(&mWatcher, &QFileSystemWatcher::directoryChanged, &mRebuildDelayTimer, QOverload<>::of(&QTimer::start));


    connect(this, &XdgMenuPrivate::changed, q_ptr, &XdgMenu::changed);
}


const QString XdgMenu::logDir() const
{
    Q_D(const XdgMenu);
    return d->mLogDir;
}


void XdgMenu::setLogDir(const QString& directory)
{
    Q_D(XdgMenu);
    d->mLogDir = directory;
}


const QDomDocument XdgMenu::xml() const
{
    Q_D(const XdgMenu);
    return d->mXml;
}


QString XdgMenu::menuFileName() const
{
    Q_D(const XdgMenu);
    return d->mMenuFileName;
}


QStringList XdgMenu::environments()
{
    Q_D(XdgMenu);
    return d->mEnvironments;
}


void XdgMenu::setEnvironments(const QStringList &envs)
{
    Q_D(XdgMenu);
    d->mEnvironments = envs;
}


void XdgMenu::setEnvironments(const QString &env)
{
    setEnvironments(QStringList() << env);
}


const QString XdgMenu::errorString() const
{
    Q_D(const XdgMenu);
    return d->mErrorString;
}


bool XdgMenu::read(const QString& menuFileName)
{
    Q_D(XdgMenu);

    d->mMenuFileName = menuFileName;

    d->clearWatcher();

    XdgMenuReader reader(this);
    if (!reader.load(d->mMenuFileName))
    {
        qWarning() << reader.errorString();
        d->mErrorString = reader.errorString();
        return false;
    }

    d->mXml = reader.xml();
    QDomElement root = d->mXml.documentElement();
    d->saveLog("00-reader.xml"_L1);

    d->simplify(root);
    d->saveLog("01-simplify.xml"_L1);

    d->mergeMenus(root);
    d->saveLog("02-mergeMenus.xml"_L1);

    d->moveMenus(root);
    d->saveLog("03-moveMenus.xml"_L1);

    d->mergeMenus(root);
    d->saveLog("04-mergeMenus.xml"_L1);

    d->deleteDeletedMenus(root);
    d->saveLog("05-deleteDeletedMenus.xml"_L1);

    d->processDirectoryEntries(root, QStringList());
    d->saveLog("06-processDirectoryEntries.xml"_L1);

    d->processApps(root);
    d->saveLog("07-processApps.xml"_L1);

    d->processLayouts(root);
    d->saveLog("08-processLayouts.xml"_L1);

    d->deleteEmpty(root);
    d->saveLog("09-deleteEmpty.xml"_L1);

    d->fixSeparators(root);
    d->saveLog("10-fixSeparators.xml"_L1);


    d->mOutDated = false;
    d->mHash = QCryptographicHash::hash(d->mXml.toByteArray(), QCryptographicHash::Md5);

    return true;
}


void XdgMenu::save(const QString& fileName)
{
    Q_D(const XdgMenu);

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        qWarning() << "Cannot write file %1:\n%2."_L1
                .arg(fileName, file.errorString());
        return;
    }

    QTextStream ts(&file);
    d->mXml.save(ts, 2);

    file.close();
}


/************************************************
 For debug only
 ************************************************/
void XdgMenuPrivate::load(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        qWarning() << "%1 not loading: %2"_L1.arg(fileName, file.errorString());
        return;
    }
    mXml.setContent(&file, QDomDocument::ParseOption::UseNamespaceProcessing);
}


void XdgMenuPrivate::saveLog(const QString& logFileName)
{
    Q_Q(XdgMenu);
    if (!mLogDir.isEmpty())
        q->save(mLogDir + u'/' + logFileName);
}


void XdgMenuPrivate::mergeMenus(QDomElement& element)
{
    QHash<QString, QDomElement> menus;

    MutableDomElementIterator it(element, "Menu"_L1);

    it.toFront();
    while(it.hasNext())
    {
        it.next();
        menus[it.current().attribute("name"_L1)] = it.current();
    }


    it.toBack();
    while (it.hasPrevious())
    {
        QDomElement src = it.previous();
        QDomElement dest = menus[src.attribute("name"_L1)];
        if (dest != src)
        {
            prependChilds(src, dest);
            element.removeChild(src);
        }
    }


    QDomElement n = element.firstChildElement("Menu"_L1);
    while (!n.isNull())
    {
        mergeMenus(n);
        n = n.nextSiblingElement("Menu"_L1);
    }

    it.toFront();
    while(it.hasNext())
        mergeMenus(it.next());
}


void XdgMenuPrivate::simplify(QDomElement& element)
{
    MutableDomElementIterator it(element);
    //it.toFront();
    while(it.hasNext())
    {
        QDomElement n = it.next();

        if (n.tagName() == "Name"_L1)
        {
            // The <Name> field must not contain the slash character ("/"_L1;
            // implementations should discard any name containing a slash.
            element.setAttribute("name"_L1, n.text().remove(u'/'));
            n.parentNode().removeChild(n);
        }

        // ......................................
        else if(n.tagName() == "Deleted"_L1)
        {
            element.setAttribute("deleted"_L1, true);
            n.parentNode().removeChild(n);
        }
        else if(n.tagName() == "NotDeleted"_L1)
        {
            element.setAttribute("deleted"_L1, false);
            n.parentNode().removeChild(n);
        }

        // ......................................
        else if(n.tagName() == "OnlyUnallocated"_L1)
        {
            element.setAttribute("onlyUnallocated"_L1, true);
            n.parentNode().removeChild(n);
        }
        else if(n.tagName() == "NotOnlyUnallocated"_L1)
        {
            element.setAttribute("onlyUnallocated"_L1, false);
            n.parentNode().removeChild(n);
        }

        // ......................................
        else if(n.tagName() == "FileInfo"_L1)
        {
            n.parentNode().removeChild(n);
        }

        // ......................................
        else if(n.tagName() == "Menu"_L1)
        {
            simplify(n);
        }

    }

}


void XdgMenuPrivate::prependChilds(QDomElement& srcElement, QDomElement& destElement)
{
    MutableDomElementIterator it(srcElement);

    it.toBack();
    while(it.hasPrevious())
    {
        QDomElement n = it.previous();
        destElement.insertBefore(n, destElement.firstChild());
    }

    if (srcElement.attributes().contains("deleted"_L1) &&
        !destElement.attributes().contains("deleted"_L1)
       )
        destElement.setAttribute("deleted"_L1, srcElement.attribute("deleted"_L1));

    if (srcElement.attributes().contains("onlyUnallocated"_L1) &&
        !destElement.attributes().contains("onlyUnallocated"_L1)
       )
        destElement.setAttribute("onlyUnallocated"_L1, srcElement.attribute("onlyUnallocated"_L1));
}


void XdgMenuPrivate::appendChilds(QDomElement& srcElement, QDomElement& destElement)
{
    MutableDomElementIterator it(srcElement);

    while(it.hasNext())
        destElement.appendChild(it.next());

    if (srcElement.attributes().contains("deleted"_L1))
        destElement.setAttribute("deleted"_L1, srcElement.attribute("deleted"_L1));

    if (srcElement.attributes().contains("onlyUnallocated"_L1))
        destElement.setAttribute("onlyUnallocated"_L1, srcElement.attribute("onlyUnallocated"_L1));
}


/************************************************
 Search item by path. The path can be absolute or relative. If the element not
 found, the behavior depends on a parameter "createNonExisting." If it's true, then
 the missing items will be created, otherwise the function returns 0.
 ************************************************/
QDomElement XdgMenu::findMenu(QDomElement& baseElement, const QString& path, bool createNonExisting)
{
    Q_D(XdgMenu);
    // Absolute path ..................
    if (path.startsWith(u'/'))
    {
        QDomElement root = d->mXml.documentElement();
        return findMenu(root, path.section(u'/', 2), createNonExisting);
    }

    // Relative path ..................
    if (path.isEmpty())
        return baseElement;


    QString name = path.section(u'/', 0, 0);
    MutableDomElementIterator it(baseElement);
    while(it.hasNext())
    {
        QDomElement n = it.next();
        if (n.attribute("name"_L1) == name)
            return findMenu(n, path.section(u'/', 1), createNonExisting);
    }



    // Not found ......................
    if (!createNonExisting)
        return QDomElement();


    const QStringList names = path.split(u'/', Qt::SkipEmptyParts);
    QDomElement el = baseElement;
    for (const QString &n : names)
    {
        QDomElement p = el;
        el = d->mXml.createElement("Menu"_L1);
        p.appendChild(el);
        el.setAttribute("name"_L1, n);
    }
    return el;

}


bool isParent(const QDomElement& parent, const QDomElement& child)
{
    QDomNode  n = child;
    while (!n.isNull())
    {
        if (n == parent)
            return true;
        n = n.parentNode();
    }
    return false;
}


/************************************************
 Recurse to resolve <Move> elements for each menu starting with any child menu before
 handling the more top level menus. So the deepest menus have their <Move> operations
 performed first. Within each <Menu>, execute <Move> operations in the order that
 they appear.
 If the destination path does not exist, simply relocate the origin <Menu> element,
 and change its <Name> field to match the destination path.
 If the origin path does not exist, do nothing.
 If both paths exist, take the origin <Menu> element, delete its <Name> element, and
 prepend its remaining child elements to the destination <Menu> element.
 ************************************************/
void XdgMenuPrivate::moveMenus(QDomElement& element)
{
    Q_Q(XdgMenu);

    {
        MutableDomElementIterator i(element, "Menu"_L1);
        while(i.hasNext())
            moveMenus(i.next());
    }

    MutableDomElementIterator i(element, "Move"_L1);
    while(i.hasNext())
    {
        i.next();
        QString oldPath = i.current().lastChildElement("Old"_L1).text();
        QString newPath = i.current().lastChildElement("New"_L1).text();

        element.removeChild(i.current());

        if (oldPath.isEmpty() || newPath.isEmpty())
            continue;

        QDomElement oldMenu = q->findMenu(element, oldPath, false);
        if (oldMenu.isNull())
            continue;

        QDomElement newMenu = q->findMenu(element, newPath, true);

        if (isParent(oldMenu, newMenu))
            continue;

        appendChilds(oldMenu, newMenu);
        oldMenu.parentNode().removeChild(oldMenu);
    }
}


/************************************************
 For each <Menu> containing a <Deleted> element which is not followed by a
 <NotDeleted> element, remove that menu and all its child menus.

 Kmenuedit create .hidden menu entry, delete it too.
 ************************************************/
void XdgMenuPrivate::deleteDeletedMenus(QDomElement& element)
{
    MutableDomElementIterator i(element, "Menu"_L1);
    while(i.hasNext())
    {
        QDomElement e = i.next();
        if (e.attribute("deleted"_L1) == "1"_L1 ||
            e.attribute("name"_L1) == ".hidden"_L1
            )
            element.removeChild(e);
        else
            deleteDeletedMenus(e);
    }

}


void XdgMenuPrivate::processDirectoryEntries(QDomElement& element, const QStringList& parentDirs)
{
    QStringList dirs;
    QStringList files;

    element.setAttribute("title"_L1, element.attribute("name"_L1));

    MutableDomElementIterator i(element, QString());
    i.toBack();
    while(i.hasPrevious())
    {
        QDomElement e = i.previous();

        if (e.tagName() == "Directory"_L1)
        {
            files << e.text();
            element.removeChild(e);
        }

        else if (e.tagName() == "DirectoryDir"_L1)
        {
            dirs << e.text();
            element.removeChild(e);
        }
    }

    dirs << parentDirs;

    bool found = false;
    for (const QString &file : std::as_const(files)){
        if (file.startsWith(u'/'))
            found = loadDirectoryFile(file, element);
        else
        {
            for (const QString &dir : std::as_const(dirs))
            {
                found = loadDirectoryFile(dir + u'/' + file, element);
                if (found) break;
            }
        }
        if (found) break;
    }


    MutableDomElementIterator it(element, "Menu"_L1);
    while(it.hasNext())
    {
        QDomElement e = it.next();
        processDirectoryEntries(e, dirs);
    }

}


bool XdgMenuPrivate::loadDirectoryFile(const QString& fileName, QDomElement& element)
{
    XdgDesktopFile file;
    file.load(fileName);

    if (!file.isValid())
        return false;


    element.setAttribute("title"_L1, file.localizedValue("Name"_L1).toString());
    element.setAttribute("comment"_L1, file.localizedValue("Comment"_L1).toString());
    element.setAttribute("icon"_L1, file.value("Icon"_L1).toString());

    Q_Q(XdgMenu);
    q->addWatchPath(QFileInfo(file.fileName()).absolutePath());
    return true;
}


void XdgMenuPrivate::processApps(QDomElement& element)
{
    Q_Q(XdgMenu);
    XdgMenuApplinkProcessor processor(element, q);
    processor.run();
}


void XdgMenuPrivate::deleteEmpty(QDomElement& element)
{
    MutableDomElementIterator it(element, "Menu"_L1);
    while(it.hasNext())
        deleteEmpty(it.next());

    if (element.attribute("keep"_L1) == "true"_L1)
        return;

    QDomElement childMenu = element.firstChildElement("Menu"_L1);
    QDomElement childApps = element.firstChildElement("AppLink"_L1);

    if (childMenu.isNull() && childApps.isNull())
    {
        element.parentNode().removeChild(element);
    }
}


void XdgMenuPrivate::processLayouts(QDomElement& element)
{
    XdgMenuLayoutProcessor proc(element);
    proc.run();
}


void XdgMenuPrivate::fixSeparators(QDomElement& element)
{

    MutableDomElementIterator it(element, "Separator"_L1);
    while(it.hasNext())
    {
        QDomElement s = it.next();
        if (s.previousSiblingElement().tagName() == "Separator"_L1)
            element.removeChild(s);
    }


    QDomElement first = element.firstChild().toElement();
    if (first.tagName() == "Separator"_L1)
        element.removeChild(first);

    QDomElement last = element.lastChild().toElement();
    if (last.tagName() == "Separator"_L1)
        element.removeChild(last);


    MutableDomElementIterator mi(element, "Menu"_L1);
    while(mi.hasNext())
        fixSeparators(mi.next());
}


/************************************************
 $XDG_CONFIG_DIRS/menus/${XDG_MENU_PREFIX}applications.menu
 The first file found in the search path should be used; other files are ignored.
 ************************************************/
QString XdgMenu::getMenuFileName(const QString& baseName)
{
    const QStringList configDirs = XdgDirs::configDirs();
    QString menuPrefix = QString::fromLocal8Bit(qgetenv("XDG_MENU_PREFIX"));

    for (const QString &configDir : configDirs)
    {
        QFileInfo file("%1/menus/%2%3"_L1.arg(configDir, menuPrefix, baseName));
        if (file.exists())
            return file.filePath();
    }

    QStringList wellKnownFiles;
    // razor- is a priority for us
    wellKnownFiles << "razor-applications.menu"_L1;
    // the "global" menu file name on suse and fedora
    wellKnownFiles << "applications.menu"_L1;
    // rest files ordered by priority (descending)
    wellKnownFiles << "kde4-applications.menu"_L1;
    wellKnownFiles << "kde-applications.menu"_L1;
    wellKnownFiles << "plasma-applications.menu"_L1;
    wellKnownFiles << "gnome-applications.menu"_L1;
    wellKnownFiles << "lxde-applications.menu"_L1;

    for (const QString &configDir : configDirs)
    {
        for (const QString &f : std::as_const(wellKnownFiles))
        {
            QFileInfo file("%1/menus/%2"_L1.arg(configDir, f));
            if (file.exists())
                return file.filePath();
        }
    }


    return QString();
}


void XdgMenu::addWatchPath(const QString &path)
{
    Q_D(XdgMenu);

    if (d->mWatcher.files().contains(path))
        return;

    if (d->mWatcher.directories().contains(path))
        return;

    d->mWatcher.addPath(path);
}


bool XdgMenu::isOutDated() const
{
    Q_D(const XdgMenu);
    return d->mOutDated;
}


void XdgMenuPrivate::rebuild()
{
    Q_Q(XdgMenu);
    QByteArray prevHash = mHash;
    q->read(mMenuFileName);

    if (prevHash != mHash)
    {
        mOutDated = true;
        Q_EMIT changed();
    }
}


void XdgMenuPrivate::clearWatcher()
{
    QStringList sl;
    sl << mWatcher.files();
    sl << mWatcher.directories();
    if (sl.length())
        mWatcher.removePaths(sl);
}
