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

#include "xdgmenureader.h"
#include "xdgmenu.h"
#include "xdgdirs.h"
#include "xmlhelper.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QtXml/QDomNamedNodeMap>
#include <QtXml/QDomNode>

using namespace Qt::Literals::StringLiterals;

XdgMenuReader::XdgMenuReader(XdgMenu* menu, XdgMenuReader*  parentReader, QObject *parent) :
    QObject(parent),
    mMenu(menu)
{
    mParentReader = parentReader;
    if (mParentReader)
        mBranchFiles << mParentReader->mBranchFiles;
}


XdgMenuReader::~XdgMenuReader() = default;


bool XdgMenuReader::load(const QString& fileName, const QString& baseDir)
{
    if (fileName.isEmpty())
    {
        mErrorStr = "Menu file not defined."_L1;
        return false;
    }

    QFileInfo fileInfo(QDir(baseDir), fileName);

    mFileName = fileInfo.canonicalFilePath();
    mDirName = fileInfo.canonicalPath();

    if (mBranchFiles.contains(mFileName))
        return false; // Recursive loop detected

    mBranchFiles << mFileName;

    QFile file(mFileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        mErrorStr = "%1 not loading: %2"_L1.arg(fileName, file.errorString());
        return false;
    }
    //qDebug() << "Load file:" << mFileName;
    mMenu->addWatchPath(mFileName);

    QDomDocument::ParseResult res = mXml.setContent(&file, QDomDocument::ParseOption::UseNamespaceProcessing);
    if (!res)
    {
        mErrorStr = QString::fromLatin1("Parse error at line %1, column %2:\n%3")
                        .arg(res.errorLine)
                        .arg(res.errorColumn)
                        .arg(res.errorMessage);
       return false;
    }

    QDomElement root = mXml.documentElement();

    QDomElement debugElement = mXml.createElement("FileInfo"_L1);
    debugElement.setAttribute("file"_L1, mFileName);
    if (mParentReader)
        debugElement.setAttribute("parent"_L1, mParentReader->fileName());

    QDomNode null;
    root.insertBefore(debugElement, null);

    processMergeTags(root);
    return true;
}


/************************************************
 Duplicate <MergeXXX> elements (that specify the same file) are handled as with
 duplicate <AppDir> elements (the last duplicate is used).
 ************************************************/
void XdgMenuReader::processMergeTags(QDomElement& element)
{
    QDomElement n = element.lastChildElement();
    QStringList mergedFiles;


    while (!n.isNull())
    {
        QDomElement next = n.previousSiblingElement();
        // MergeFile ..................
        if (n.tagName() == "MergeFile"_L1)
        {
            processMergeFileTag(n, &mergedFiles);
            n.parentNode().removeChild(n);
        }

        // MergeDir ...................
        else if(n.tagName() == "MergeDir"_L1)
        {
            processMergeDirTag(n, &mergedFiles);
            n.parentNode().removeChild(n);
        }

        // DefaultMergeDirs ...........
        else if (n.tagName() == "DefaultMergeDirs"_L1)
        {
            processDefaultMergeDirsTag(n, &mergedFiles);
            n.parentNode().removeChild(n);
        }

        // AppDir ...................
        else if(n.tagName() == "AppDir"_L1)
        {
            processAppDirTag(n);
            n.parentNode().removeChild(n);
        }

        // DefaultAppDirs .............
        else if(n.tagName() == "DefaultAppDirs"_L1)
        {
            processDefaultAppDirsTag(n);
            n.parentNode().removeChild(n);
        }

        // DirectoryDir ...................
        else if(n.tagName() == "DirectoryDir"_L1)
        {
            processDirectoryDirTag(n);
            n.parentNode().removeChild(n);
        }

        // DefaultDirectoryDirs ...........
        else if(n.tagName() == "DefaultDirectoryDirs"_L1)
        {
            processDefaultDirectoryDirsTag(n);
            n.parentNode().removeChild(n);
        }


        // Menu .......................
        else if(n.tagName() == "Menu"_L1)
        {
            processMergeTags(n);
        }

        n = next;
    }

}


/************************************************
 Any number of <MergeFile> elements may be listed below a <Menu> element, giving
 the name of another menu file to be merged into this one.
 If fileName is not an absolute path then the file to be merged should be located
 relative to the location of this menu.

 If the type attribute is missing or set to "path" then the contents of the
 <MergeFile> element indicates the file to be merged.

 If the type attribute is set to "parent" and the file that contains this
 <MergeFile> element is located under one of the paths specified by
 $XDG_CONFIG_DIRS, the contents of the element should be ignored and the remaining
 paths specified by $XDG_CONFIG_DIRS are searched for a file with the same relative
 filename. The first file encountered should be merged. There should be no merging
 at all if no matching file is found. ( Libmenu additional scans ~/.config/menus.)
 ************************************************/
void XdgMenuReader::processMergeFileTag(QDomElement& element, QStringList* mergedFiles)
{
    //qDebug() << "Process " << element;// << "in" << mFileName;

    if (element.attribute("type"_L1) != "parent"_L1)
    {
        mergeFile(element.text(), element, mergedFiles);
    }

    else
    {
        QString relativeName;
        QStringList configDirs = XdgDirs::configDirs();

        for (const QString &configDir : std::as_const(configDirs))
        {
            if (mFileName.startsWith(configDir))
            {
                relativeName = mFileName.mid(configDir.length());
                configDirs.removeAll(configDir);
                break;
            }
        }


        if (relativeName.isEmpty())
        {
            QString configHome = XdgDirs::configHome();
            if (mFileName.startsWith(configHome))
                relativeName = mFileName.mid(configHome.length());
        }

        if (relativeName.isEmpty())
            return;

        for (const QString &configDir : std::as_const(configDirs))
        {
            if (QFileInfo::exists(configDir + relativeName))
            {
                mergeFile(configDir + relativeName, element, mergedFiles);
                return;
            }
        }
    }
}


/************************************************
 A <MergeDir> contains the name of a directory. Each file in the given directory
 which ends in the ".menu" extension should be merged in the same way that a
 <MergeFile> would be. If the filename given as a <MergeDir> is not an absolute
 path, it should be located relative to the location of the menu file being parsed.
 The files inside the merged directory are not merged in any specified order.

 Duplicate <MergeDir> elements (that specify the same directory) are handled as with
 duplicate <AppDir> elements (the last duplicate is used).

 KDE additional scans ~/.config/menus.
 ************************************************/
void XdgMenuReader::processMergeDirTag(QDomElement& element, QStringList* mergedFiles)
{
    //qDebug() << "Process " << element;// << "in" << mFileName;

    mergeDir(element.text(), element, mergedFiles);
    element.parentNode().removeChild(element);
}


/************************************************
 The element has no content. The element should be treated as if it were a list of
 <MergeDir> elements containing the default merge directory locations. When expanding
 <DefaultMergeDirs> to a list of <MergeDir>, the default locations that are earlier
 in the search path go later in the <Menu> so that they have priority.

 Note that a system that uses either gnome-applications.menu or kde-applications.menu
 depending on the desktop environment in use must still use applications-merged as the
 default merge directory in both cases.

 Implementations may chose to use .menu files with names other than application.menu
 for tasks or menus other than the main application menu. In that case the first part
 of the name of the default merge directory is derived from the name of the .menu file.
 ************************************************/
void XdgMenuReader::processDefaultMergeDirsTag(QDomElement& element, QStringList* mergedFiles)
{
    //qDebug() << "Process " << element;// << "in" << mFileName;

    QString menuBaseName = QFileInfo(mMenu->menuFileName()).baseName();
    int n = menuBaseName.lastIndexOf(u'-');
    if (n>-1)
        menuBaseName = menuBaseName.mid(n+1);

    QStringList dirs = XdgDirs::configDirs();
    dirs << XdgDirs::configHome();

    for (const QString &dir : std::as_const(dirs))
    {
        mergeDir("%1/menus/%2-merged"_L1.arg(dir, menuBaseName), element, mergedFiles);
    }

    if (menuBaseName == "applications"_L1)
        mergeFile("%1/menus/applications-kmenuedit.menu"_L1.arg(XdgDirs::configHome()), element, mergedFiles);
}


/************************************************
 If the filename given as an <AppDir> is not an absolute path, it should be located
 relative to the location of the menu file being parsed.
 ************************************************/
void XdgMenuReader::processAppDirTag(QDomElement& element)
{
    //qDebug() << "Process " << element;
    addDirTag(element, "AppDir"_L1, element.text());
}


/************************************************
 The element has no content. The element should be treated as if it were a list of
 <AppDir> elements containing the default app dir locations
 ($XDG_DATA_DIRS/applications/).

 menu-cache additional prepends $XDG_DATA_HOME/applications.
 ************************************************/
void XdgMenuReader::processDefaultAppDirsTag(QDomElement& element)
{
    //qDebug() << "Process " << element;
    QStringList dirs = XdgDirs::dataDirs();
    dirs.prepend(XdgDirs::dataHome(false));

    for (const QString &dir : std::as_const(dirs))
    {
        //qDebug() << "Add AppDir: " << dir + "/applications/";
        addDirTag(element, "AppDir"_L1, dir + "/applications/"_L1);
    }
}

/************************************************
 If the filename given as a <DirectoryDir> is not an absolute path, it should be
 located relative to the location of the menu file being parsed.
 ************************************************/
void XdgMenuReader::processDirectoryDirTag(QDomElement& element)
{
    //qDebug() << "Process " << element;
    addDirTag(element, "DirectoryDir"_L1, element.text());
}


/************************************************
 The element has no content. The element should be treated as if it were a list of
 <DirectoryDir> elements containing the default desktop dir locations
 ($XDG_DATA_DIRS/desktop-directories/).

 The default locations that are earlier in the search path go later in the <Menu>
 so that they have priority.

 menu-cache additional prepends $XDG_DATA_HOME/applications.
 ************************************************/
void XdgMenuReader::processDefaultDirectoryDirsTag(QDomElement& element)
{
    //qDebug() << "Process " << element;
    QStringList dirs = XdgDirs::dataDirs();
    dirs.prepend(XdgDirs::dataHome(false));

    int n = dirs.size();
    for (int i = 0; i < n; ++i)
        addDirTag(element, "DirectoryDir"_L1, dirs.at(n - i - 1) + "/desktop-directories/"_L1);
}

/************************************************

 ************************************************/
void XdgMenuReader::addDirTag(QDomElement& previousElement, const QString& tagName, const QString& dir)
{
    QFileInfo dirInfo(mDirName, dir);
    if (dirInfo.isDir())
    {
//        qDebug() << "\tAdding " + dirInfo.canonicalFilePath();
        QDomElement element = mXml.createElement(tagName);
        element.appendChild(mXml.createTextNode(dirInfo.canonicalFilePath()));
        previousElement.parentNode().insertBefore(element, previousElement);
    }
}

/*
 If fileName is not an absolute path then the file to be merged should be located
 relative to the location of this menu file.
 ************************************************/
void XdgMenuReader::mergeFile(const QString& fileName, QDomElement& element, QStringList* mergedFiles)
{
    XdgMenuReader reader(mMenu, this);
    QFileInfo fileInfo(QDir(mDirName), fileName);

    if (!fileInfo.exists())
        return;

    if (mergedFiles->contains(fileInfo.canonicalFilePath()))
    {
        //qDebug() << "\tSkip: allredy merged";
        return;
    }

    //qDebug() << "Merge file: " << fileName;
    mergedFiles->append(fileInfo.canonicalFilePath());

    if (reader.load(fileName, mDirName))
    {
        //qDebug() << "\tOK";
        QDomElement n = reader.xml().firstChildElement().firstChildElement();
        while (!n.isNull())
        {
            // As a special exception, remove the <Name> element from the root
            // element of each file being merged.
            if (n.tagName() != "Name"_L1)
            {
                QDomNode imp = mXml.importNode(n, true);
                element.parentNode().insertBefore(imp, element);
            }

            n = n.nextSiblingElement();
        }
    }
}


void XdgMenuReader::mergeDir(const QString& dirName, QDomElement& element, QStringList* mergedFiles)
{
    QFileInfo dirInfo(mDirName, dirName);

    if (dirInfo.isDir())
    {
        //qDebug() << "Merge dir: " << dirInfo.canonicalFilePath();
        QDir dir = QDir(dirInfo.canonicalFilePath());
        const QFileInfoList files = dir.entryInfoList(QStringList() << "*.menu"_L1, QDir::Files | QDir::Readable);

        for (const QFileInfo &file : files)
            mergeFile(file.canonicalFilePath(), element, mergedFiles);
    }
}
