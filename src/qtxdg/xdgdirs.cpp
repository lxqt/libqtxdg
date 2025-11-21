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

// clazy:excludeall=non-pod-global-static

#include "xdgdirs.h"
#include <cstdlib>
#include <QDir>
#include <QDebug>
#include <QLatin1StringView>
#include <QStandardPaths>


using namespace Qt::Literals::StringLiterals;

static constexpr QLatin1StringView userDirectoryString[8] =
{
    "Desktop"_L1,
    "Download"_L1,
    "Templates"_L1,
    "Publicshare"_L1,
    "Documents"_L1,
    "Music"_L1,
    "Pictures"_L1,
    "Videos"_L1
};

// Helper functions prototypes
void fixBashShortcuts(QString &s);
void removeEndingSlash(QString &s);
QString createDirectory(const QString &dir);

void cleanAndAddPostfix(QStringList &dirs, const QString& postfix);
QString handlePostfix(const QString &postfix);
QString userDirFallback(XdgDirs::UserDirectory dir);

/************************************************
 Helper func.
 ************************************************/
void fixBashShortcuts(QString &s)
{
    if (s.startsWith(u'~'))
        s = QFile::decodeName(qgetenv("HOME")) + (s).mid(1);
}


void removeEndingSlash(QString &s)
{
    // We don't check for empty strings. Caller must check it.

    // Remove the ending slash, except for root dirs.
    if (s.length() > 1 && s.endsWith(u'/'))
        s.chop(1);
}


QString createDirectory(const QString &dir)
{
    QDir d(dir);
    if (!d.exists())
    {
        if (!d.mkpath("."_L1))
        {
            qWarning() << "Can't create %1 directory."_L1.arg(d.absolutePath());
            return QString();
        }
    }
    QString r = d.absolutePath();
    removeEndingSlash(r);
    return r;
}


void cleanAndAddPostfix(QStringList &dirs, const QString& postfix)
{
    const QString universalPostfix = handlePostfix(postfix);
    const int N = dirs.count();
    for(int i = 0; i < N; ++i)
    {
        fixBashShortcuts(dirs[i]);
        removeEndingSlash(dirs[i]);
        dirs[i].append(universalPostfix);
    }
}

QString handlePostfix(const QString &postfix)
{
    if (postfix.isEmpty())
        return QString();

    if (postfix.startsWith(u'/'))
        return postfix;

    return u'/' + postfix;
}


QString userDirFallback(XdgDirs::UserDirectory dir)
{
    QString fallback;
    const QString home = QFile::decodeName(qgetenv("HOME"));

    if (home.isEmpty())
        return "/tmp"_L1;
    else if (dir == XdgDirs::Desktop)
        fallback = "%1/%2"_L1.arg(home, "Desktop"_L1);
    else
        fallback = home;

    return fallback;
}


QString XdgDirs::userDirDefault(XdgDirs::UserDirectory dir)
{
    // possible values for UserDirectory
    Q_ASSERT(!(dir < XdgDirs::Desktop || dir > XdgDirs::Videos));
    if (dir < XdgDirs::Desktop || dir > XdgDirs::Videos)
        return QString();

    return userDirFallback(dir);
}


QString XdgDirs::userDir(XdgDirs::UserDirectory dir)
{
    // possible values for UserDirectory
    Q_ASSERT(!(dir < XdgDirs::Desktop || dir > XdgDirs::Videos));
    if (dir < XdgDirs::Desktop || dir > XdgDirs::Videos)
        return QString();

    QString folderName = userDirectoryString[dir];

    const QString fallback = userDirFallback(dir);

    QString configDir(configHome());
    QFile configFile(configDir + "/user-dirs.dirs"_L1);
    if (!configFile.exists())
        return fallback;

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return fallback;

    QString userDirVar("XDG_"_L1 + folderName.toUpper() + "_DIR"_L1);
    QTextStream in(&configFile);
    QString line;
    while (!in.atEnd())
    {
        line = in.readLine();
        if (line.contains(userDirVar))
        {
            configFile.close();

            // get path between quotes
            line = line.section(u'"', 1, 1);
            if (line.isEmpty())
                return fallback;
            line.replace("$HOME"_L1, "~"_L1);
            fixBashShortcuts(line);
            return line;
        }
    }

    configFile.close();
    return fallback;
}


bool XdgDirs::setUserDir(XdgDirs::UserDirectory dir, const QString& value, bool createDir)
{
    // possible values for UserDirectory
    Q_ASSERT(!(dir < XdgDirs::Desktop || dir > XdgDirs::Videos));
    if (dir < XdgDirs::Desktop || dir > XdgDirs::Videos)
        return false;

    const QString home = QFile::decodeName(qgetenv("HOME"));
    if (!(value.startsWith("$HOME"_L1)
                           || value.startsWith("~/"_L1)
                           || value.startsWith(home)
                           || value.startsWith(QDir(home).canonicalPath())))
	return false;

    QString folderName = userDirectoryString[dir];

    QString configDir(configHome());
    QFile configFile(configDir + "/user-dirs.dirs"_L1);

    // create the file if doesn't exist and opens it
    if (!configFile.open(QIODevice::ReadWrite | QIODevice::Text))
        return false;

    QTextStream stream(&configFile);
    QVector<QString> lines;
    QString line;
    bool foundVar = false;
    while (!stream.atEnd())
    {
        line = stream.readLine();
        if (line.indexOf("XDG_"_L1 + folderName.toUpper() + "_DIR"_L1) == 0)
        {
            foundVar = true;
            QString path = line.section(u'"', 1, 1);
            line.replace(path, value);
            lines.append(line);
        }
        else if (line.indexOf("XDG_"_L1) == 0)
        {
            lines.append(line);
        }
    }

    stream.reset();
    configFile.resize(0);
    if (!foundVar)
        stream << "XDG_%1_DIR=\"%2\"\n"_L1.arg(folderName.toUpper(),(value));

    for (QVector<QString>::iterator i = lines.begin(); i != lines.end(); ++i)
        stream << *i << u'\n';

    configFile.close();

    if (createDir) {
        QString path = QString(value).replace("$HOME"_L1, "~"_L1);
        fixBashShortcuts(path);

        return createDirectory(path).isEmpty() ? false : true;
    }

    return true;
}


QString XdgDirs::dataHome(bool createDir)
{
    QString s = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    fixBashShortcuts(s);
    if (createDir)
        return createDirectory(s);

   removeEndingSlash(s);
   return s;
}


QString XdgDirs::configHome(bool createDir)
{
    QString s = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    fixBashShortcuts(s);
    if (createDir)
        return createDirectory(s);

   removeEndingSlash(s);
   return s;
}


QStringList XdgDirs::dataDirs(const QString &postfix)
{
    QString d = QFile::decodeName(qgetenv("XDG_DATA_DIRS"));
    QStringList dirs = d.split(u':', Qt::SkipEmptyParts);

    if (dirs.isEmpty()) {
        dirs.append("/usr/local/share"_L1);
        dirs.append("/usr/share"_L1);
    } else {
        QMutableListIterator<QString> it(dirs);
        while (it.hasNext()) {
            const QString dir = it.next();
            if (!dir.startsWith(u'/'))
                it.remove();
        }
    }

    dirs.removeDuplicates();
    cleanAndAddPostfix(dirs, postfix);
    return dirs;

}


QStringList XdgDirs::configDirs(const QString &postfix)
{
    QStringList dirs;
    const QString env = QFile::decodeName(qgetenv("XDG_CONFIG_DIRS"));
    if (env.isEmpty())
        dirs.append("/etc/xdg"_L1);
    else
        dirs = env.split(u':', Qt::SkipEmptyParts);

    cleanAndAddPostfix(dirs, postfix);
    return dirs;
}


QString XdgDirs::cacheHome(bool createDir)
{
    QString s = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    fixBashShortcuts(s);
    if (createDir)
        return createDirectory(s);

    removeEndingSlash(s);
    return s;
}


QString XdgDirs::runtimeDir()
{
    QString result = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    fixBashShortcuts(result);
    removeEndingSlash(result);
    return result;
}


QString XdgDirs::autostartHome(bool createDir)
{
    const QString confHome = configHome(createDir);

    if (confHome.isEmpty())
        return QString();

    QString s = confHome + "/autostart"_L1;
    fixBashShortcuts(s);

    if (createDir)
        return createDirectory(s);

     QDir d(s);
     QString r = d.absolutePath();
     removeEndingSlash(r);
     return r;
}


QStringList XdgDirs::autostartDirs(const QString &postfix)
{
    QStringList dirs;
    const QStringList s = configDirs();
    for (const QString &dir : s)
        dirs << "%1/autostart"_L1.arg(dir);

    cleanAndAddPostfix(dirs, postfix);
    return dirs;
}
