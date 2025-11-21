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

// clazy:excludeall=non-pod-global-static

#ifndef QT_NO_ICON
#include "xdgiconloader_p.h"

#include <private/qguiapplication_p.h>
#include <private/qicon_p.h>

#include <QtGui/QIconEnginePlugin>
#include <QtGui/QPixmapCache>
#include <qpa/qplatformtheme.h>
#include <QtGui/QIconEngine>
#include <QtGui/QPalette>
#include <QtCore/qmath.h>
#include <QtCore/QList>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QStringView>
#include <QtGui/QPainter>
#include <QImageReader>
#include <QXmlStreamReader>
#include <QFileSystemWatcher>
#include <QSvgRenderer>

#include <private/qhexstring_p.h>

using namespace Qt::Literals::StringLiterals;

Q_GLOBAL_STATIC(XdgIconLoader, iconLoaderInstance)

/* Theme to use in last resort, if the theme does not have the icon, neither the parents  */
static QString fallbackTheme()
{
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme()) {
        const QVariant themeHint = theme->themeHint(QPlatformTheme::SystemIconFallbackThemeName);
        if (themeHint.isValid()) {
            const QString theme = themeHint.toString();
            if (theme != "hicolor"_L1)
                return theme;
        }
    }
    return QString();
}

#ifdef QT_NO_LIBRARY
static bool gSupportsSvg = false;
#else
static bool gSupportsSvg = true;
#endif //QT_NO_LIBRARY

void XdgIconLoader::setFollowColorScheme(bool enable)
{
    if (m_followColorScheme != enable)
    {
        QIconLoader::instance()->invalidateKey();
        m_followColorScheme = enable;
    }
}

XdgIconLoader *XdgIconLoader::instance()
{
   QIconLoader::instance()->ensureInitialized();
   return iconLoaderInstance();
}

/*!
    \class QIconCacheGtkReader
    \internal
    Helper class that reads and looks up into the icon-theme.cache generated with
    gtk-update-icon-cache. If at any point we detect a corruption in the file
    (because the offsets point at wrong locations for example), the reader
    is marked as invalid.
*/
class QIconCacheGtkReader
{
public:
    explicit QIconCacheGtkReader(const QString &themeDir);
    QList<const char *> lookup(QStringView);
    bool isValid() const { return m_isValid; }
    bool reValid(bool infoRefresh);
private:
    QFileInfo m_cacheFileInfo;
    QFile m_file;
    const unsigned char *m_data;
    quint64 m_size;
    bool m_isValid;

    quint16 read16(uint offset)
    {
        if (offset > m_size - 2 || (offset & 0x1)) {
            m_isValid = false;
            return 0;
        }
        return m_data[offset+1] | m_data[offset] << 8;
    }
    quint32 read32(uint offset)
    {
        if (offset > m_size - 4 || (offset & 0x3)) {
            m_isValid = false;
            return 0;
        }
        return m_data[offset+3] | m_data[offset+2] << 8
            | m_data[offset+1] << 16 | m_data[offset] << 24;
    }
};
Q_GLOBAL_STATIC(QFileSystemWatcher, gtkCachesWatcher)


QIconCacheGtkReader::QIconCacheGtkReader(const QString &dirName)
    : m_cacheFileInfo{dirName + "/icon-theme.cache"_L1}
    , m_data(nullptr)
    , m_isValid(false)
{
    m_file.setFileName(m_cacheFileInfo.absoluteFilePath());
    // Note: The cache file can be (IS) removed and newly created during the
    // cache update. But we hold open file descriptor for the "old" removed
    // file. So we need to watch the changes and reopen/remap the file.
    gtkCachesWatcher->addPath(dirName);
    QObject::connect(gtkCachesWatcher(), &QFileSystemWatcher::directoryChanged, &m_file, [this]
        {
            m_isValid = false;
            // invalidate icons to reload them ...
            QIconLoader::instance()->invalidateKey();
        });
    reValid(false);
}

bool QIconCacheGtkReader::reValid(bool infoRefresh)
{
    if (m_data)
        m_file.unmap(const_cast<unsigned char *>(m_data));
    m_file.close();

    if (infoRefresh)
        m_cacheFileInfo.refresh();

    const QDir dir = m_cacheFileInfo.absoluteDir();

    if (!m_cacheFileInfo.exists() || m_cacheFileInfo.lastModified() < QFileInfo{dir.absolutePath()}.lastModified())
        return m_isValid;

    if (!m_file.open(QFile::ReadOnly))
        return m_isValid;
    m_size = m_file.size();
    m_data = m_file.map(0, m_size);
    if (!m_data)
        return m_isValid;
    if (read16(0) != 1) // VERSION_MAJOR
        return m_isValid;

    m_isValid = true;

    // Check that all the directories are older than the cache
    auto lastModified = m_cacheFileInfo.lastModified();
    quint32 dirListOffset = read32(8);
    quint32 dirListLen = read32(dirListOffset);
    for (uint i = 0; i < dirListLen; ++i) {
        quint32 offset = read32(dirListOffset + 4 + 4 * i);
        if (!m_isValid || offset >= m_size || lastModified < QFileInfo(dir
                    , QString::fromUtf8(reinterpret_cast<const char*>(m_data + offset))).lastModified()) {
            m_isValid = false;
            return m_isValid;
        }
    }
    return m_isValid;
}

static quint32 icon_name_hash(const char *p)
{
    quint32 h = static_cast<signed char>(*p);
    for (p += 1; *p != '\0'; p++)
        h = (h << 5) - h + *p;
    return h;
}

/*! \internal
    lookup the icon name and return the list of subdirectories in which an icon
    with this name is present. The char* are pointers to the mapped data.
    For example, this would return { "32x32/apps", "24x24/apps" , ... }
 */

QList<const char *> QIconCacheGtkReader::lookup(QStringView name)
{
    QList<const char *> ret;
    if (!isValid() || name.isEmpty())
        return ret;

    QByteArray nameUtf8 = name.toUtf8();
    quint32 hash = icon_name_hash(nameUtf8.data());

    quint32 hashOffset = read32(4);
    quint32 hashBucketCount = read32(hashOffset);

    if (!isValid() || hashBucketCount == 0) {
        m_isValid = false;
        return ret;
    }

    quint32 bucketIndex = hash % hashBucketCount;
    quint32 bucketOffset = read32(hashOffset + 4 + bucketIndex * 4);
    while (bucketOffset > 0 && bucketOffset <= m_size - 12) {
        quint32 nameOff = read32(bucketOffset + 4);
        if (nameOff < m_size && strcmp(reinterpret_cast<const char*>(m_data + nameOff), nameUtf8.constData()) == 0) {
            quint32 dirListOffset = read32(8);
            quint32 dirListLen = read32(dirListOffset);

            quint32 listOffset = read32(bucketOffset+8);
            quint32 listLen = read32(listOffset);

            if (!m_isValid || listOffset + 4 + 8 * listLen > m_size) {
                m_isValid = false;
                return ret;
            }

            ret.reserve(listLen);
            for (uint j = 0; j < listLen && m_isValid; ++j) {
                quint32 dirIndex = read16(listOffset + 4 + 8 * j);
                quint32 o = read32(dirListOffset + 4 + dirIndex*4);
                if (!m_isValid || dirIndex >= dirListLen || o >= m_size) {
                    m_isValid = false;
                    return ret;
                }
                ret.append(reinterpret_cast<const char*>(m_data) + o);
            }
            return ret;
        }
        bucketOffset = read32(bucketOffset);
    }
    return ret;
}

XdgIconTheme::XdgIconTheme(const QString &themeName)
        : m_valid(false)
        , m_followsColorScheme(false)
{
    QFile themeIndex;

    const QStringList iconDirs = QIcon::themeSearchPaths();
    for ( int i = 0 ; i < iconDirs.size() ; ++i) {
        QDir iconDir(iconDirs[i]);
        QString themeDir = iconDir.path() + u'/' + themeName;
        QFileInfo themeDirInfo(themeDir);

        if (themeDirInfo.isDir()) {
            m_contentDirs << themeDir;
            m_gtkCaches << QSharedPointer<QIconCacheGtkReader>::create(themeDir);
        }

        if (!m_valid) {
            themeIndex.setFileName(themeDir + "/index.theme"_L1);
            if (themeIndex.exists())
                m_valid = true;
        }
    }
#ifndef QT_NO_SETTINGS
    if (themeIndex.exists()) {
        const QSettings indexReader(themeIndex.fileName(), QSettings::IniFormat);
        m_followsColorScheme = indexReader.value(u"Icon Theme/FollowsColorScheme"_s, false).toBool();
        const QStringList keys = indexReader.allKeys();
        for (auto const &key : keys) {
            if (key.endsWith("/Size"_L1)) {
                // Note the QSettings ini-format does not accept
                // slashes in key names, hence we have to cheat
                if (int size = indexReader.value(key).toInt()) {
                    QString directoryKey = key.left(key.size() - 5);
                    QIconDirInfo dirInfo(directoryKey);
                    dirInfo.size = size;
                    QString type = indexReader.value(directoryKey +
                                                     "/Type"_L1
                                                     ).toString();

                    if (type == "Fixed"_L1)
                        dirInfo.type = QIconDirInfo::Fixed;
                    else if (type == "Scalable"_L1)
                        dirInfo.type = QIconDirInfo::Scalable;
                    else
                        dirInfo.type = QIconDirInfo::Threshold;

                    dirInfo.threshold = indexReader.value(directoryKey +
                                                        "/Threshold"_L1,
                                                        2).toInt();

                    dirInfo.minSize = indexReader.value(directoryKey +
                                                         "/MinSize"_L1,
                                                         size).toInt();

                    dirInfo.maxSize = indexReader.value(directoryKey +
                                                        "/MaxSize"_L1,
                                                        size).toInt();
                    dirInfo.scale = indexReader.value(directoryKey +
                                                      "/Scale"_L1,
                                                      1).toInt();
                    m_keyList.append(dirInfo);
                }
            }
        }

        // Parent themes provide fallbacks for missing icons
        m_parents = indexReader.value(
                "Icon Theme/Inherits"_L1).toStringList();
        m_parents.removeAll(QString());

        // Ensure a default platform fallback for all themes
        if (m_parents.isEmpty()) {
            const QString fallback = fallbackTheme();
            if (!fallback.isEmpty())
                m_parents.append(fallback);
        }
    }
#endif //QT_NO_SETTINGS
}

/* WARNING:
 *
 * https://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
 *
 * <cite>
 * The dash “-” character is used to separate levels of specificity in icon
 * names, for all contexts other than MimeTypes. For instance, we use
 * “input-mouse” as the generic item for all mouse devices, and we use
 * “input-mouse-usb” for a USB mouse device. However, if the more specific
 * item does not exist in the current theme, and does exist in a parent
 * theme, the generic icon from the current theme is preferred, in order
 * to keep consistent style.
 * </cite>
 *
 * But we believe, that using the more specific icon (even from parents)
 * is better for user experience. So we are violating the standard
 * intentionally.
 *
 * Ref.
 * https://github.com/lxqt/lxqt/issues/1252
 * https://github.com/lxqt/libqtxdg/pull/116
 */
QThemeIconInfo XdgIconLoader::findIconHelper(const QString &themeName,
                                 const QString &iconName,
                                 QStringList &visited,
                                 bool dashFallback) const
{
    QThemeIconInfo info;
    Q_ASSERT(!themeName.isEmpty());

    // Used to protect against potential recursions
    visited << themeName;

    XdgIconTheme &theme = themeList[themeName];
    if (!theme.isValid()) {
        theme = XdgIconTheme(themeName);
        if (!theme.isValid()) {
            const QString fallback = fallbackTheme();
            if (!fallback.isEmpty())
                theme = XdgIconTheme(fallback);
        }
    }

    const QStringList contentDirs = theme.contentDirs();

    const QString svgext(".svg"_L1);
    const QString pngext(".png"_L1);
    const QString xpmext(".xpm"_L1);


    QStringView iconNameFallback(iconName);

    // Iterate through all icon's fallbacks in current theme
    if (info.entries.empty()) {
        const QString svgIconName = iconNameFallback + svgext;
        const QString pngIconName = iconNameFallback + pngext;
        const QString xpmIconName = iconNameFallback + xpmext;

        // Add all relevant files
        for (int i = 0; i < contentDirs.size(); ++i) {
            QList<QIconDirInfo> subDirs = theme.keyList();

            // Try to reduce the amount of subDirs by looking in the GTK+ cache in order to save
            // a massive amount of file stat (especially if the icon is not there)
            auto cache = theme.m_gtkCaches.at(i);
            if (cache->isValid() || cache->reValid(true)) {
                const auto result = cache->lookup(iconNameFallback);
                if (cache->isValid()) {
                    const QList<QIconDirInfo> subDirsCopy = subDirs;
                    subDirs.clear();
                    subDirs.reserve(result.count());
                    for (const char *s : result) {
                        QString path = QString::fromUtf8(s);
                        auto it = std::find_if(subDirsCopy.cbegin(), subDirsCopy.cend(),
                                               [&](const QIconDirInfo &info) {
                                                   return info.path == path; } );
                        if (it != subDirsCopy.cend()) {
                            subDirs.append(*it);
                        }
                    }
                }
            }

            QString contentDir = contentDirs.at(i) + u'/';
            for (int j = 0; j < subDirs.size() ; ++j) {
                const QIconDirInfo &dirInfo = subDirs.at(j);
                const QString subDir = contentDir + dirInfo.path + u'/';
                const QString pngPath = subDir + pngIconName;
                if (QFile::exists(pngPath)) {
                    auto iconEntry = std::make_unique<PixmapEntry>();
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = pngPath;
                    // Notice we ensure that pixmap entries always come before
                    // scalable to preserve search order afterwards
                    info.entries.insert(info.entries.begin(), std::move(iconEntry));
                } else if (gSupportsSvg) {
                    const QString svgPath = subDir + svgIconName;
                    if (QFile::exists(svgPath)) {
                        std::unique_ptr<QIconLoaderEngineEntry> iconEntry;
                        if (followColorScheme() && theme.followsColorScheme())
                            iconEntry.reset(new ScalableFollowsColorEntry);
                        else
                            iconEntry.reset(new ScalableEntry);
                        iconEntry->dir = dirInfo;
                        iconEntry->filename = svgPath;
                        info.entries.push_back(std::move(iconEntry));
                    }
                }
                const QString xpmPath = subDir + xpmIconName;
                if (QFile::exists(xpmPath)) {
                    auto iconEntry = std::make_unique<PixmapEntry>();
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = xpmPath;
                    // Notice we ensure that pixmap entries always come before
                    // scalable to preserve search order afterwards
                    info.entries.insert(info.entries.begin(), std::move(iconEntry));
                }
            }
        }

        if (!info.entries.empty())
            info.iconName = iconNameFallback.toString();
    }

    if (info.entries.empty()) {
        const QStringList parents = theme.parents();
        // Search recursively through inherited themes
        for (int i = 0 ; i < parents.size() ; ++i) {

            const QString parentTheme = parents.at(i).trimmed();

            if (!visited.contains(parentTheme)) // guard against recursion
                info = findIconHelper(parentTheme, iconName, visited);

            if (!info.entries.empty()) // success
                break;
        }

        // make sure that hicolor is also searched before dash fallbacks
        if (info.entries.empty()
            && !parents.contains("hicolor"_L1)
            && !visited.contains("hicolor"_L1)) {
            info = findIconHelper("hicolor"_L1, iconName, visited);
        }
    }

    if (info.entries.empty()) {
        // Also, consider Qt's fallback search paths (which are not defined by Freedesktop)
        // if the icon is not found in any inherited theme
        const auto fallbackPaths = QIcon::fallbackSearchPaths();
        for (const auto &fallbackPath : fallbackPaths) {
            const QString pngPath = fallbackPath + u'/' + iconName + pngext;
            if (QFile::exists(pngPath)) {
                auto iconEntry = std::make_unique<PixmapEntry>();
                QIconDirInfo dirInfo(fallbackPath);
                iconEntry->dir = dirInfo;
                iconEntry->filename = pngPath;
                info.entries.insert(info.entries.begin(), std::move(iconEntry));
            } else {
                const QString svgPath = fallbackPath + u'/' + iconName + svgext;
                if (gSupportsSvg && QFile::exists(svgPath)) {
                    auto iconEntry = std::make_unique<ScalableEntry>();
                    QIconDirInfo dirInfo(fallbackPath);
                    iconEntry->dir = dirInfo;
                    iconEntry->filename = svgPath;
                    info.entries.push_back(std::move(iconEntry));
                }
            }
        }
    }

    if (dashFallback && info.entries.empty()) {
        // If it's possible - find next fallback for the icon
        const int indexOfDash = iconNameFallback.lastIndexOf(u'-');
        if (indexOfDash != -1) {
            iconNameFallback.truncate(indexOfDash);
            QStringList _visited;
            info = findIconHelper(themeName, iconNameFallback.toString(), _visited, true);
        }
    }

    return info;
}

QThemeIconInfo XdgIconLoader::unthemedFallback(const QString &iconName, const QStringList &searchPaths) const
{
    QThemeIconInfo info;

    const QString svgext(".svg"_L1);
    const QString pngext(".png"_L1);
    const QString xpmext(".xpm"_L1);

    for (const auto &contentDir : searchPaths)  {
        QDir currentDir(contentDir);

        if (currentDir.exists(iconName + pngext)) {
            auto iconEntry = std::make_unique<PixmapEntry>();
            iconEntry->filename = currentDir.filePath(iconName + pngext);
            // Notice we ensure that pixmap entries always come before
            // scalable to preserve search order afterwards
            info.entries.insert(info.entries.begin(), std::move(iconEntry));
        } else if (gSupportsSvg &&
            currentDir.exists(iconName + svgext)) {
            auto iconEntry = std::make_unique<ScalableEntry>();
            iconEntry->filename = currentDir.filePath(iconName + svgext);
            info.entries.push_back(std::move(iconEntry));
        } else if (currentDir.exists(iconName + xpmext)) {
            auto iconEntry = std::make_unique<PixmapEntry>();
            iconEntry->filename = currentDir.filePath(iconName + xpmext);
            // Notice we ensure that pixmap entries always come before
            // scalable to preserve search order afterwards
            info.entries.insert(info.entries.begin(), std::move(iconEntry));
        }
    }
    return info;
}

QThemeIconInfo XdgIconLoader::loadIcon(const QString &name) const
{
    const QString theme_name = QIconLoader::instance()->themeName();
    if (!theme_name.isEmpty()) {
        QStringList visited;
        auto info = findIconHelper(theme_name, name, visited, true);
        if (info.entries.empty()) {
            auto unthemedInfo = unthemedFallback(name, QIcon::themeSearchPaths());
            if (unthemedInfo.entries.empty()) {
                /* Freedesktop standard says to look in /usr/share/pixmaps last */
                const QStringList pixmapPath = (QStringList() << "/usr/share/pixmaps"_L1);
                auto pixmapInfo = unthemedFallback(name, pixmapPath);
                if (pixmapInfo.entries.empty()) {
                    return QThemeIconInfo();
                } else {
                    return pixmapInfo;
                }
            } else {
                return unthemedInfo;
            }
        } else {
            return info;
        }
    }

    return QThemeIconInfo();
}


// -------- Icon Loader Engine -------- //


XdgIconLoaderEngine::XdgIconLoaderEngine(const QString& iconName)
        : m_iconName(iconName), m_key(0)
{
}

XdgIconLoaderEngine::~XdgIconLoaderEngine() = default;

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
    return !(m_info.entries.empty());
}

// Lazily load the icon
void XdgIconLoaderEngine::ensureLoaded()
{
    if (QIconLoader::instance()->themeKey() != m_key) {
        m_info = XdgIconLoader::instance()->loadIcon(m_iconName);
        m_key = QIconLoader::instance()->themeKey();
    }
}

void XdgIconLoaderEngine::paint(QPainter *painter, const QRect &rect,
                                QIcon::Mode mode, QIcon::State state)
{
    QSize pixmapSize = rect.size();
    auto paintDevice = painter->device();
    const qreal dpr = paintDevice ? paintDevice->devicePixelRatioF() : qApp->devicePixelRatio();
    painter->drawPixmap(rect, pixmap(QSizeF(pixmapSize * dpr).toSize(), mode, state));
}

/*
 * This algorithm is defined by the freedesktop spec:
 * http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html
 */
static bool directoryMatchesSize(const QIconDirInfo &dir, int iconsize, int iconscale)
{
    if (dir.scale != iconscale)
        return false;
    if (dir.type == QIconDirInfo::Fixed) {
        return dir.size == iconsize;

    } else if (dir.type == QIconDirInfo::Scalable) {
        return iconsize <= dir.maxSize &&
                iconsize >= dir.minSize;

    } else if (dir.type == QIconDirInfo::Threshold) {
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
static int directorySizeDistance(const QIconDirInfo &dir, int iconsize, int iconscale)
{
    const int scaledIconSize = iconsize * iconscale;
    if (dir.type == QIconDirInfo::Fixed) {
        return qAbs(dir.size * dir.scale - scaledIconSize);

    } else if (dir.type == QIconDirInfo::Scalable) {
        if (scaledIconSize < dir.minSize * dir.scale)
            return dir.minSize * dir.scale - scaledIconSize;
        else if (scaledIconSize > dir.maxSize * dir.scale)
            return scaledIconSize - dir.maxSize * dir.scale;
        else
            return 0;

    } else if (dir.type == QIconDirInfo::Threshold) {
        if (scaledIconSize < (dir.size - dir.threshold) * dir.scale)
            return dir.minSize * dir.scale - scaledIconSize;
        else if (scaledIconSize > (dir.size + dir.threshold) * dir.scale)
            return scaledIconSize - dir.maxSize * dir.scale;
        else return 0;
    }

    Q_ASSERT(1); // Not a valid value
    return INT_MAX;
}

QIconLoaderEngineEntry *XdgIconLoaderEngine::entryForSize(const QThemeIconInfo &info, const QSize &size, int scale)
{
    int iconsize = qMin(size.width(), size.height());

    // Note that info.entries are sorted so that png-files
    // come first

    // Search for exact matches first
    for (const auto &entry : info.entries) {
        if (directoryMatchesSize(entry->dir, iconsize, scale)) {
            return entry.get();
        }
    }

    // Find the minimum distance icon
    int minimalSize = INT_MAX;
    QIconLoaderEngineEntry *closestMatch = nullptr;
    for (const auto &entry : info.entries) {
        int distance = directorySizeDistance(entry->dir, iconsize, scale);
        if (distance < minimalSize) {
            minimalSize  = distance;
            closestMatch = entry.get();
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
    Q_UNUSED(mode);
    Q_UNUSED(state);

    ensureLoaded();

    QIconLoaderEngineEntry *entry = entryForSize(m_info, size);
    if (entry) {
        const QIconDirInfo &dir = entry->dir;
        if (dir.type == QIconDirInfo::Scalable
            || dynamic_cast<ScalableEntry *>(entry)
            || dynamic_cast<ScalableFollowsColorEntry *>(entry)) {
            return size;
        }
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
            return {result, result};
        }
    }
    return {0, 0};
}

// XXX: duplicated from qiconloader.cpp, because this symbol isn't exported :(
#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
QPixmap PixmapEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
#else
QPixmap PixmapEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
#endif
{
    Q_UNUSED(state);

    // Ensure that basePixmap is lazily initialized before generating the
    // key, otherwise the cache key is not unique
    if (basePixmap.isNull())
        basePixmap.load(filename);

    // see QPixmapIconEngine::adjustSize
    QSize actualSize = basePixmap.size();
    // If the size of the best match we have (basePixmap) is larger than the
    // requested size, we downscale it to match.
    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);

#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
    // see QIconPrivate::pixmapDevicePixelRatio
    qreal calculatedDpr;
    QSize targetSize = size * scale;
    if ((actualSize.width() == targetSize.width() && actualSize.height() <= targetSize.height()) ||
        (actualSize.width() <= targetSize.width() && actualSize.height() == targetSize.height()))
    {
        // Correctly scaled for dpr, just having different aspect ratio
        calculatedDpr = scale;
    }
    else
    {
        qreal ratio = 0.5 * (qreal(actualSize.width()) / qreal(targetSize.width()) +
                             qreal(actualSize.height() / qreal(targetSize.height())));
        calculatedDpr = qMax(qreal(1.0), scale * ratio);
    }

    QString key = "$qt_theme_"_L1
                  % HexString<quint64>(basePixmap.cacheKey())
                  % HexString<quint8>(mode)
                  % HexString<quint64>(QGuiApplication::palette().cacheKey())
                  % HexString<uint>(actualSize.width())
                  % HexString<uint>(actualSize.height())
                  % HexString<quint16>(qRound(calculatedDpr * 1000));
#else
    QString key = "$qt_theme_"_L1
                  % HexString<qint64>(basePixmap.cacheKey())
                  % HexString<int>(mode)
                  % HexString<qint64>(QGuiApplication::palette().cacheKey())
                  % HexString<int>(actualSize.width())
                  % HexString<int>(actualSize.height());
#endif

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
#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
        cachedPixmap.setDevicePixelRatio(calculatedDpr);
#endif
        QPixmapCache::insert(key, cachedPixmap);
    }
    return cachedPixmap;
}

// NOTE: For SVG, QSvgRenderer is used to prevent our icon handling from
// being broken by icon engines that register themselves for SVG.
#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
QPixmap ScalableEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
#else
QPixmap ScalableEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
#endif
{
    QPixmap pm;
    if (size.isEmpty())
        return pm;

#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
    QString key = "lxqt_"_L1
                  % filename
                  % HexString<quint8>(mode)
                  % HexString<int>(state)
                  % HexString<uint>(size.width())
                  % HexString<uint>(size.height())
                  % HexString<quint16>(qRound(scale * 1000));
#else
    QString key = "lxqt_"_L1
                  % filename
                  % HexString<int>(mode)
                  % HexString<int>(state)
                  % HexString<int>(size.width())
                  % HexString<int>(size.height());
#endif
    if (!QPixmapCache::find(key, &pm))
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
        int icnSize = qMin(size.width(), size.height()) * scale;
#else
        int icnSize = qMin(size.width(), size.height());
#endif
        pm = QPixmap(icnSize, icnSize);
        pm.fill(Qt::transparent);

        QSvgRenderer renderer;
        if (renderer.load(filename))
        {
            QPainter p;
            p.begin(&pm);
            renderer.render(&p, QRect(0, 0, icnSize, icnSize));
            p.end();
        }

        svgIcon = QIcon(pm);
#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
        pm = svgIcon.pixmap(size, scale, mode, state);
#else
        if (QIconEngine *engine = svgIcon.data_ptr() ? svgIcon.data_ptr()->engine : nullptr)
            pm = engine->pixmap(size, mode, state);
#endif
        QPixmapCache::insert(key, pm);
    }

    return pm;
}

static const QString STYLE = u"\n.ColorScheme-Text, .ColorScheme-NeutralText {color:%1;}\
\n.ColorScheme-Background {color:%2;}\
\n.ColorScheme-Highlight {color:%3;}"_s;
// NOTE: Qt palette does not have any colors for positive/negative text
// .ColorScheme-PositiveText,ColorScheme-NegativeText {color:%4;}

#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
QPixmap ScalableFollowsColorEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
#else
QPixmap ScalableFollowsColorEntry::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
#endif
{
    QPixmap pm;
    if (size.isEmpty())
        return pm;

    const QPalette pal = qApp->palette();
    QString txtCol, bgCol, hCol;
    if (mode == QIcon::Disabled)
    {
        txtCol = pal.color(QPalette::Disabled, QPalette::WindowText).name();
        bgCol = pal.color(QPalette::Disabled, QPalette::Window).name();
        hCol = pal.color(QPalette::Disabled, QPalette::Highlight).name();
    }
    else
    {
        if (mode == QIcon::Selected)
        {
            txtCol = pal.highlightedText().color().name();
            bgCol = pal.highlight().color().name();
        }
        else // normal or active
        {
            txtCol = pal.windowText().color().name();
            bgCol = pal.window().color().name();
        }
        hCol = pal.highlight().color().name();
    }
#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
    QString key = "lxqt_"_L1
                  % filename
                  % HexString<quint8>(mode)
                  % HexString<int>(state)
                  % HexString<uint>(size.width())
                  % HexString<uint>(size.height())
                  % HexString<quint16>(qRound(scale * 1000))
                  % txtCol % bgCol % hCol;
#else
    QString key = "lxqt_"_L1
                  % filename
                  % HexString<int>(mode)
                  % HexString<int>(state)
                  % HexString<int>(size.width())
                  % HexString<int>(size.height())
                  % txtCol % bgCol % hCol;
#endif
    if (!QPixmapCache::find(key, &pm))
    {
#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
        int icnSize = qMin(size.width(), size.height()) * scale;
#else
        int icnSize = qMin(size.width(), size.height());
#endif
        pm = QPixmap(icnSize, icnSize);
        pm.fill(Qt::transparent);

        QFile device{filename};
        if (device.open(QIODevice::ReadOnly))
        {
            QString styleSheet = STYLE.arg(txtCol, bgCol, hCol);
            QByteArray svgBuffer;
            QXmlStreamWriter writer(&svgBuffer);
            QXmlStreamReader xmlReader(&device);
            while (!xmlReader.atEnd())
            {
                if (xmlReader.readNext() == QXmlStreamReader::StartElement
                    && xmlReader.qualifiedName() == "style"_L1
                    && xmlReader.attributes().value("id"_L1) == "current-color-scheme"_L1)
                {
                    const auto attribs = xmlReader.attributes();
                    // store original data/text of the <style> element
                    QString origData;
                    while (xmlReader.tokenType() != QXmlStreamReader::EndElement)
                    {
                        if (xmlReader.tokenType() == QXmlStreamReader::Characters)
                            origData += xmlReader.text();
                        xmlReader.readNext();
                    }
                    writer.writeStartElement("style"_L1);
                    writer.writeAttributes(attribs);
                    writer.writeCharacters(origData);
                    writer.writeCharacters(styleSheet);
                    writer.writeEndElement();
                }
                else if (xmlReader.tokenType() != QXmlStreamReader::Invalid)
                    writer.writeCurrentToken(xmlReader);
            }

            if (!svgBuffer.isEmpty())
            {
                QSvgRenderer renderer;
                renderer.load(svgBuffer);
                QPainter p;
                p.begin(&pm);
                renderer.render(&p, QRect(0, 0, icnSize, icnSize));
                p.end();
            }
        }

        // Do not use this pixmap directly but first get the icon
        // for QIcon::pixmap() to handle states and modes,
        // especially the disabled mode.
        svgIcon = QIcon(pm);
#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
        pm = svgIcon.pixmap(size, scale, mode, state);
#else
        if (QIconEngine *engine = svgIcon.data_ptr() ? svgIcon.data_ptr()->engine : nullptr)
            pm = engine->pixmap(size, mode, state);
#endif
        QPixmapCache::insert(key, pm);
    }

    return pm;
}

QPixmap XdgIconLoaderEngine::pixmap(const QSize &size, QIcon::Mode mode,
                                 QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

QString XdgIconLoaderEngine::key() const
{
    return "XdgIconLoaderEngine"_L1;
}

QString XdgIconLoaderEngine::iconName()
{
    ensureLoaded();
    return m_info.iconName;
}

bool XdgIconLoaderEngine::isNull()
{
    ensureLoaded();
    return m_info.entries.empty();
}

QPixmap XdgIconLoaderEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    ensureLoaded();
    const int integerScale = qCeil(scale);
#if (QT_VERSION >= QT_VERSION_CHECK(6,8,0))
    QIconLoaderEngineEntry *entry = entryForSize(m_info, size, integerScale);
    return entry ? entry->pixmap(size, mode, state, scale) : QPixmap();
#else
    QIconLoaderEngineEntry *entry = entryForSize(m_info, size / integerScale, integerScale);
    return entry ? entry->pixmap(size, mode, state) : QPixmap();
#endif
}

QList<QSize> XdgIconLoaderEngine::availableSizes(QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(mode);
    Q_UNUSED(state);
    ensureLoaded();
    const int N = m_info.entries.size();
    QList<QSize> sizes;
    sizes.reserve(N);

    // Gets all sizes from the DirectoryInfo entries
    for (const auto &entry : m_info.entries) {
        if (entry->dir.type == QIconDirInfo::Fallback) {
            sizes.append(QIcon(entry->filename).availableSizes());
        } else {
            int size = entry->dir.size;
            sizes.append(QSize(size, size));
        }
    }
    return sizes;
}

#endif //QT_NO_ICON
