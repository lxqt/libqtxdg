/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QT_NO_ICON
#include "xdgiconloader_p.h"

#include <private/qguiapplication_p.h>
#include <private/qicon_p.h>

#include <QtGui/QIconEnginePlugin>
#include <QtGui/QPixmapCache>
#include <qpa/qplatformtheme.h>
#include <QtGui/QIconEngine>
#include <QtGui/QPalette>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtGui/QPainter>

#ifdef Q_WS_MAC
#include <private/qt_cocoa_helpers_mac_p.h>
#endif

#include <private/qhexstring_p.h>

//QT_BEGIN_NAMESPACE


Q_GLOBAL_STATIC(XdgIconLoader, iconLoaderInstance)

/* Theme to use in last resort, if the theme does not have the icon, neither the parents  */
static QString fallbackTheme()
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(QPlatformTheme::SystemIconFallbackThemeName);
        if (themeHint.isValid())
            return themeHint.toString();
    }
    return QString("hicolor");
}

XdgIconLoader::XdgIconLoader() :
        m_themeKey(1), m_supportsSvg(false), m_initialized(false)
{
}

// We lazily initialize the loader to make static icons
// work. Though we do not officially support this.

static inline QString systemThemeName()
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(QPlatformTheme::SystemIconThemeName);
        if (themeHint.isValid())
            return themeHint.toString();
    }
    return QIcon::themeName();
}

static inline QStringList systemIconSearchPaths()
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(QPlatformTheme::IconThemeSearchPaths);
        if (themeHint.isValid())
            return themeHint.toStringList();
    }
    return QIcon::themeSearchPaths();
}

#ifndef QT_NO_LIBRARY
//extern QFactoryLoader *qt_iconEngineFactoryLoader(); // qicon.cpp
#endif

void XdgIconLoader::ensureInitialized()
{
    if (!m_initialized) {
        m_initialized = true;

        Q_ASSERT(qApp);

        m_systemTheme = systemThemeName();

        if (m_systemTheme.isEmpty())
            m_systemTheme = fallbackTheme();
#ifndef QT_NO_LIBRARY
//        if (qt_iconEngineFactoryLoader()->keyMap().key(QLatin1String("svg"), -1) != -1)
            m_supportsSvg = true;
#endif //QT_NO_LIBRARY
    }
}

XdgIconLoader *XdgIconLoader::instance()
{
   iconLoaderInstance()->ensureInitialized();
   return iconLoaderInstance();
}

// Queries the system theme and invalidates existing
// icons if the theme has changed.
void XdgIconLoader::updateSystemTheme()
{
    // Only change if this is not explicitly set by the user
    if (m_userTheme.isEmpty()) {
        QString theme = systemThemeName();
        if (theme.isEmpty())
            theme = fallbackTheme();
        if (theme != m_systemTheme) {
            m_systemTheme = theme;
            invalidateKey();
        }
    }
}

void XdgIconLoader::setThemeName(const QString &themeName)
{
    m_userTheme = themeName;
    invalidateKey();
}

void XdgIconLoader::setThemeSearchPath(const QStringList &searchPaths)
{
    m_iconDirs = searchPaths;
    themeList.clear();
    invalidateKey();
}

QStringList XdgIconLoader::themeSearchPaths() const
{
    if (m_iconDirs.isEmpty()) {
        m_iconDirs = systemIconSearchPaths();
        // Always add resource directory as search path
        m_iconDirs.append(QLatin1String(":/icons"));
    }
    return m_iconDirs;
}

QIconTheme::QIconTheme(const QString &themeName)
        : m_valid(false)
{
    QFile themeIndex;

    QStringList iconDirs = QIcon::themeSearchPaths();
    for ( int i = 0 ; i < iconDirs.size() ; ++i) {
        QDir iconDir(iconDirs[i]);
        QString themeDir = iconDir.path() + QLatin1Char('/') + themeName;
        themeIndex.setFileName(themeDir + QLatin1String("/index.theme"));
        if (themeIndex.exists()) {
            m_contentDirs << themeDir;
            m_valid = true;

            QStringList themeSearchPaths = QIcon::themeSearchPaths();
            foreach (QString path, themeSearchPaths)
            {
                if (!path.startsWith(':') && QFileInfo(path).isDir())
                    m_contentDirs.append(path + QLatin1Char('/') + themeName);
            }

            break;
        }
    }
#ifndef QT_NO_SETTINGS
    if (themeIndex.exists()) {
        const QSettings indexReader(themeIndex.fileName(), QSettings::IniFormat);
        QStringListIterator keyIterator(indexReader.allKeys());
        while (keyIterator.hasNext()) {

            const QString key = keyIterator.next();
            if (key.endsWith(QLatin1String("/Size"))) {
                // Note the QSettings ini-format does not accept
                // slashes in key names, hence we have to cheat
                if (int size = indexReader.value(key).toInt()) {
                    QString directoryKey = key.left(key.size() - 5);
                    XdgIconDirInfo dirInfo(directoryKey);
                    dirInfo.size = size;
                    QString type = indexReader.value(directoryKey +
                                                     QLatin1String("/Type")
                                                     ).toString();

                    if (type == QLatin1String("Fixed"))
                        dirInfo.type = XdgIconDirInfo::Fixed;
                    else if (type == QLatin1String("Scalable"))
                        dirInfo.type = XdgIconDirInfo::Scalable;
                    else
                        dirInfo.type = XdgIconDirInfo::Threshold;

                    dirInfo.threshold = indexReader.value(directoryKey +
                                                        QLatin1String("/Threshold"),
                                                        2).toInt();

                    dirInfo.minSize = indexReader.value(directoryKey +
                                                         QLatin1String("/MinSize"),
                                                         size).toInt();

                    dirInfo.maxSize = indexReader.value(directoryKey +
                                                        QLatin1String("/MaxSize"),
                                                        size).toInt();
                    m_keyList.append(dirInfo);
                }
            }
        }

        // Parent themes provide fallbacks for missing icons
        m_parents = indexReader.value(
                QLatin1String("Icon Theme/Inherits")).toStringList();
        m_parents.removeAll(QString());

        // Ensure a default platform fallback for all themes
        if (m_parents.isEmpty()) {
            const QString fallback = fallbackTheme();
            if (!fallback.isEmpty())
                m_parents.append(fallback);
        }

        // Ensure that all themes fall back to hicolor
        if (!m_parents.contains(QLatin1String("hicolor")))
            m_parents.append(QLatin1String("hicolor"));
    }
#endif //QT_NO_SETTINGS
}

QThemeIconInfo XdgIconLoader::findIconHelper(const QString &themeName,
                                 const QString &iconName,
                                 QStringList &visited) const
{
    QThemeIconInfo info;
    Q_ASSERT(!themeName.isEmpty());

    QPixmap pixmap;

    // Used to protect against potential recursions
    visited << themeName;

    QIconTheme theme = themeList.value(themeName);
    if (!theme.isValid()) {
        theme = QIconTheme(themeName);
        if (!theme.isValid())
            theme = QIconTheme(fallbackTheme());

        themeList.insert(themeName, theme);
    }

    const QStringList contentDirs = theme.contentDirs();
    const QVector<XdgIconDirInfo> subDirs = theme.keyList();

    const QString svgext(QLatin1String(".svg"));
    const QString pngext(QLatin1String(".png"));
    const QString xpmext(QLatin1String(".xpm"));


    QString iconNameFallback = iconName;

    // Iterate through all icon's fallbacks in current theme
    while (info.entries.isEmpty()) {
        const QString svgIconName = iconNameFallback + svgext;
        const QString pngIconName = iconNameFallback + pngext;
        const QString xpmIconName = iconNameFallback + xpmext;

        // Add all relevant files
        for (int i = 0; i < contentDirs.size(); ++i) {
            QString contentDir = contentDirs.at(i) + QLatin1Char('/');
            for (int j = 0; j < subDirs.size() ; ++j) {
                const XdgIconDirInfo &dirInfo = subDirs.at(j);
                QString subdir = dirInfo.path;
                QDir currentDir(contentDir + subdir);
                if (currentDir.exists(pngIconName)) {
                    PixmapEntry *iconEntry = new PixmapEntry;
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = currentDir.filePath(pngIconName);
                    // Notice we ensure that pixmap entries always come before
                    // scalable to preserve search order afterwards
                    info.entries.prepend(iconEntry);
                } else if (m_supportsSvg &&
                    currentDir.exists(svgIconName)) {
                    ScalableEntry *iconEntry = new ScalableEntry;
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = currentDir.filePath(svgIconName);
                    info.entries.append(iconEntry);
                } else if(currentDir.exists(iconName + xpmext)) {
                    PixmapEntry *iconEntry = new PixmapEntry;
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = currentDir.filePath(iconName + xpmext);
                    // Notice we ensure that pixmap entries always come before
                    // scalable to preserve search order afterwards
                    info.entries.append(iconEntry);
                    break;
                }
            }
        }

        if (!info.entries.isEmpty()) {
            info.iconName = iconNameFallback;
            break;
        }

        // If it's possible - find next fallback for the icon
        const int indexOfDash = iconNameFallback.lastIndexOf(QLatin1Char('-'));
        if (indexOfDash == -1)
            break;

        iconNameFallback.truncate(indexOfDash);
    }

    if (info.entries.isEmpty()) {
        const QStringList parents = theme.parents();
        // Search recursively through inherited themes
        for (int i = 0 ; i < parents.size() ; ++i) {

            const QString parentTheme = parents.at(i).trimmed();

            if (!visited.contains(parentTheme)) // guard against recursion
                info = findIconHelper(parentTheme, iconName, visited);

            if (!info.entries.isEmpty()) // success
                break;
        }
    }

    if (info.entries.isEmpty()) {
       // Search for unthemed icons in main dir of search paths
       QStringList themeSearchPaths = QIcon::themeSearchPaths();
        foreach (QString contentDir, themeSearchPaths)  {
            QDir currentDir(contentDir);

            if (currentDir.exists(iconName + pngext)) {
                PixmapEntry *iconEntry = new PixmapEntry;
                iconEntry->filename = currentDir.filePath(iconName + pngext);
                // Notice we ensure that pixmap entries always come before
                // scalable to preserve search order afterwards
                info.entries.prepend(iconEntry);
            } else if (m_supportsSvg &&
                currentDir.exists(iconName + svgext)) {
                ScalableEntry *iconEntry = new ScalableEntry;
                iconEntry->filename = currentDir.filePath(iconName + svgext);
                info.entries.append(iconEntry);
                break;
            } else if (currentDir.exists(iconName + xpmext)) {
                PixmapEntry *iconEntry = new PixmapEntry;
                iconEntry->filename = currentDir.filePath(iconName + xpmext);
                // Notice we ensure that pixmap entries always come before
                // scalable to preserve search order afterwards
                info.entries.append(iconEntry);
                break;
            }
        }
    }


    /*********************************************************************
    Author: Kaitlin Rupert <kaitlin.rupert@intel.com>
    Date: Aug 12, 2010
    Description: Make it so that the QIcon loader honors /usr/share/pixmaps
                 directory.  This is a valid directory per the Freedesktop.org
                 icon theme specification.
    Bug: https://bugreports.qt.nokia.com/browse/QTBUG-12874
     *********************************************************************/
#ifdef Q_OS_LINUX
    /* Freedesktop standard says to look in /usr/share/pixmaps last */
    if (info.entries.isEmpty()) {
        const QString pixmaps(QLatin1String("/usr/share/pixmaps"));

        const QDir currentDir(pixmaps);
        const XdgIconDirInfo dirInfo(pixmaps);
        if (currentDir.exists(iconName + pngext)) {
            PixmapEntry *iconEntry = new PixmapEntry;
            iconEntry->dir = dirInfo;
            iconEntry->filename = currentDir.filePath(iconName + pngext);
            // Notice we ensure that pixmap entries always come before
            // scalable to preserve search order afterwards
            info.entries.prepend(iconEntry);
        } else if (m_supportsSvg &&
                   currentDir.exists(iconName + svgext)) {
            ScalableEntry *iconEntry = new ScalableEntry;
            iconEntry->dir = dirInfo;
            iconEntry->filename = currentDir.filePath(iconName + svgext);
            info.entries.append(iconEntry);
        } else if (currentDir.exists(iconName + xpmext)) {
            PixmapEntry *iconEntry = new PixmapEntry;
            iconEntry->dir = dirInfo;
            iconEntry->filename = currentDir.filePath(iconName + xpmext);
            // Notice we ensure that pixmap entries always come before
            // scalable to preserve search order afterwards
            info.entries.append(iconEntry);
        }
    }
#endif

    return info;
}

QThemeIconInfo XdgIconLoader::loadIcon(const QString &name) const
{
    if (!themeName().isEmpty()) {
        QStringList visited;
        return findIconHelper(themeName(), name, visited);
    }

    return QThemeIconInfo();
}


// -------- Icon Loader Engine -------- //


XdgIconLoaderEngine::XdgIconLoaderEngine(const QString& iconName)
        : m_iconName(iconName), m_key(0)
{
}

XdgIconLoaderEngine::~XdgIconLoaderEngine()
{
    qDeleteAll(m_info.entries);
}

XdgIconLoaderEngine::XdgIconLoaderEngine(const XdgIconLoaderEngine &other)
        : QIconEngine(other),
        m_iconName(other.m_iconName),
        m_key(0)
{
}

QIconEngine *XdgIconLoaderEngine::clone() const
{
    return new XdgIconLoaderEngine(*this);
}

bool XdgIconLoaderEngine::read(QDataStream &in) {
    in >> m_iconName;
    return true;
}

bool XdgIconLoaderEngine::write(QDataStream &out) const
{
    out << m_iconName;
    return true;
}

bool XdgIconLoaderEngine::hasIcon() const
{
    return !(m_info.entries.isEmpty());
}

// Lazily load the icon
void XdgIconLoaderEngine::ensureLoaded()
{
    if (!(XdgIconLoader::instance()->themeKey() == m_key)) {
        qDeleteAll(m_info.entries);
        m_info.entries.clear();
        m_info.iconName.clear();

        Q_ASSERT(m_info.entries.size() == 0);
        m_info = XdgIconLoader::instance()->loadIcon(m_iconName);
        m_key = XdgIconLoader::instance()->themeKey();
    }
}

void XdgIconLoaderEngine::paint(QPainter *painter, const QRect &rect,
                             QIcon::Mode mode, QIcon::State state)
{
    QSize pixmapSize = rect.size();
#if defined(Q_WS_MAC)
    pixmapSize *= qt_mac_get_scalefactor();
#endif
    painter->drawPixmap(rect, pixmap(pixmapSize, mode, state));
}

/*
 * This algorithm is defined by the freedesktop spec:
 * http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
 */
static bool directoryMatchesSize(const XdgIconDirInfo &dir, int iconsize)
{
    if (dir.type == XdgIconDirInfo::Fixed) {
        return dir.size == iconsize;

    } else if (dir.type == XdgIconDirInfo::Scalable) {
        return dir.size <= dir.maxSize &&
                iconsize >= dir.minSize;

    } else if (dir.type == XdgIconDirInfo::Threshold) {
        return iconsize >= dir.size - dir.threshold &&
                iconsize <= dir.size + dir.threshold;
    }

    Q_ASSERT(1); // Not a valid value
    return false;
}

/*
 * This algorithm is defined by the freedesktop spec:
 * http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
 */
static int directorySizeDistance(const XdgIconDirInfo &dir, int iconsize)
{
    if (dir.type == XdgIconDirInfo::Fixed) {
        return qAbs(dir.size - iconsize);

    } else if (dir.type == XdgIconDirInfo::Scalable) {
        if (iconsize < dir.minSize)
            return dir.minSize - iconsize;
        else if (iconsize > dir.maxSize)
            return iconsize - dir.maxSize;
        else
            return 0;

    } else if (dir.type == XdgIconDirInfo::Threshold) {
        if (iconsize < dir.size - dir.threshold)
            return dir.minSize - iconsize;
        else if (iconsize > dir.size + dir.threshold)
            return iconsize - dir.maxSize;
        else return 0;
    }

    Q_ASSERT(1); // Not a valid value
    return INT_MAX;
}

XdgIconLoaderEngineEntry *XdgIconLoaderEngine::entryForSize(const QSize &size)
{
    int iconsize = qMin(size.width(), size.height());

    // Note that m_info.entries are sorted so that png-files
    // come first

    const int numEntries = m_info.entries.size();

    // Search for exact matches first
    for (int i = 0; i < numEntries; ++i) {
        XdgIconLoaderEngineEntry *entry = m_info.entries.at(i);
        if (directoryMatchesSize(entry->dir, iconsize)) {
            return entry;
        }
    }

    // Find the minimum distance icon
    int minimalSize = INT_MAX;
    XdgIconLoaderEngineEntry *closestMatch = 0;
    for (int i = 0; i < numEntries; ++i) {
        XdgIconLoaderEngineEntry *entry = m_info.entries.at(i);
        int distance = directorySizeDistance(entry->dir, iconsize);
        if (distance < minimalSize) {
            minimalSize  = distance;
            closestMatch = entry;
        }
    }
    return closestMatch;
}

/*
 * Returns the actual icon size. For scalable svg's this is equivalent
 * to the requested size. Otherwise the closest match is returned but
 * we can never return a bigger size than the requested size.
 *
 */
QSize XdgIconLoaderEngine::actualSize(const QSize &size, QIcon::Mode mode,
                                   QIcon::State state)
{
    ensureLoaded();

    XdgIconLoaderEngineEntry *entry = entryForSize(size);
    if (entry) {
        const XdgIconDirInfo &dir = entry->dir;
        if (dir.type == XdgIconDirInfo::Scalable || dynamic_cast<ScalableEntry *>(entry))
            return size;
        else {
            int dir_size = dir.size;
            //Note: fallback for directories that don't have its content size defined
            //  -> get the actual size based on the image if possible
            PixmapEntry * pix_e;
            if (0 == dir_size && nullptr != (pix_e = dynamic_cast<PixmapEntry *>(entry)))
            {
                QSize pix_size = pix_e->basePixmap.size();
                dir_size = qMin(pix_size.width(), pix_size.height());
            }
            int result = qMin(dir_size, qMin(size.width(), size.height()));
            return QSize(result, result);
        }
    }
    return QIconEngine::actualSize(size, mode, state);
}

QPixmap PixmapEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state);

    // Ensure that basePixmap is lazily initialized before generating the
    // key, otherwise the cache key is not unique
    if (basePixmap.isNull())
        basePixmap.load(filename);

    QSize actualSize = basePixmap.size();
    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);

    QString key = QLatin1String("$qt_theme_")
                  % HexString<qint64>(basePixmap.cacheKey())
                  % HexString<int>(mode)
                  % HexString<qint64>(QGuiApplication::palette().cacheKey())
                  % HexString<int>(actualSize.width())
                  % HexString<int>(actualSize.height());

    QPixmap cachedPixmap;
    if (QPixmapCache::find(key, &cachedPixmap)) {
        return cachedPixmap;
    } else {
        if (basePixmap.size() != actualSize)
            cachedPixmap = basePixmap.scaled(actualSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        else
            cachedPixmap = basePixmap;
        if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(qApp))
            cachedPixmap = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(mode, cachedPixmap);
        QPixmapCache::insert(key, cachedPixmap);
    }
    return cachedPixmap;
}

QPixmap ScalableEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    if (svgIcon.isNull())
        svgIcon = QIcon(filename);

    // Simply reuse svg icon engine
    return svgIcon.pixmap(size, mode, state);
}

QPixmap XdgIconLoaderEngine::pixmap(const QSize &size, QIcon::Mode mode,
                                 QIcon::State state)
{
    ensureLoaded();

    XdgIconLoaderEngineEntry *entry = entryForSize(size);
    if (entry)
        return entry->pixmap(size, mode, state);

    return QPixmap();
}

QString XdgIconLoaderEngine::key() const
{
    return QLatin1String("XdgIconLoaderEngine");
}

void XdgIconLoaderEngine::virtual_hook(int id, void *data)
{
    ensureLoaded();

    switch (id) {
    case QIconEngine::AvailableSizesHook:
        {
            QIconEngine::AvailableSizesArgument &arg
                    = *reinterpret_cast<QIconEngine::AvailableSizesArgument*>(data);
            const int N = m_info.entries.size();
            QList<QSize> sizes;
            sizes.reserve(N);

            // Gets all sizes from the DirectoryInfo entries
            for (int i = 0; i < N; ++i) {
                int size = m_info.entries.at(i)->dir.size;
                sizes.append(QSize(size, size));
            }
            arg.sizes.swap(sizes); // commit
        }
        break;
    case QIconEngine::IconNameHook:
        {
            QString &name = *reinterpret_cast<QString*>(data);
            name = m_iconName;
        }
        break;
    default:
        QIconEngine::virtual_hook(id, data);
    }
}


//QT_END_NAMESPACE

#endif //QT_NO_ICON
