#ifndef XDGDESKTOPFILE_P_H
#define XDGDESKTOPFILE_P_H

QTXDG_AUTOTEST bool readDesktopFile(QIODevice & device, QSettings::SettingsMap & map);
QTXDG_AUTOTEST bool writeDesktopFile(QIODevice & device, const QSettings::SettingsMap & map);

#endif // XDGDESKTOPFILE_P_H
