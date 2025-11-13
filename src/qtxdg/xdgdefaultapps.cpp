/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2020  Luís Pereira <luis.artur.pereira@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "xdgdefaultapps.h"

#include "xdgdesktopfile.h"
#include "xdgdirs.h"
#include "xdgmimeapps.h"

#include <QSet>
#include <QSettings>
#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

using namespace Qt::Literals::StringLiterals;

static XdgDesktopFile *getTerminal(const QString &terminalName)
{
    XdgDesktopFile *t = new XdgDesktopFile{};
    if (t->load(terminalName) && t->isValid()) {
        const QStringList cats = t->value("Categories"_L1, QString()).toString().split(u';', Qt::SkipEmptyParts);
        if (cats.contains("TerminalEmulator"_L1)) {
            if (t->contains("TryExec"_L1)) {
                if (t->tryExec()) {
                    return t;
                }
            } else {
                return t;
            }
        }
    }

    delete t;
    return nullptr;
}

static QStringList getWebBrowserProtocolsGet()
{
    // Protocols needed to quailify a application as the default browser
    static const QStringList webBrowserProtocolsGet = {
        "text/html"_L1,
        "x-scheme-handler/http"_L1,
        "x-scheme-handler/https"_L1
    };
    return webBrowserProtocolsGet;
}

static QStringList getWebBrowserProtocolsSet()
{
    // When setting an application as the default browser xdg-settings also
    // sets these protocols. We simply follow it as good practice.
    static const QStringList webBrowserProtocolsSet {
        "x-scheme-handler/about"_L1,
        "x-scheme-handler/unknown"_L1
    };
    return webBrowserProtocolsSet;
}

static QString qtxdgConfigFilename()
{
    // first find the DE's qtxdg.conf file
    QByteArray qtxdgConfig("qtxdg");
    QList<QByteArray> desktopsList = qgetenv("XDG_CURRENT_DESKTOP").toLower().split(':');
    if (!desktopsList.isEmpty() && !desktopsList.at(0).isEmpty()) {
        qtxdgConfig = desktopsList.at(0) + '-' + qtxdgConfig;
    }

    return QString::fromLocal8Bit(qtxdgConfig);
}

// returns the list of apps that are from category and support protocols
static QList<XdgDesktopFile *> categoryAndMimeTypeApps(const QString &category, const QStringList &protocols)
{
    XdgMimeApps db;
    QList<XdgDesktopFile *> apps = db.categoryApps(category);
    const QSet<QString> protocolsSet = QSet<QString>(protocols.begin(), protocols.end());
    QList<XdgDesktopFile*>::iterator it = apps.begin();
    while (it != apps.end()) {
        const auto list = (*it)->mimeTypes();
        const QSet<QString> appSupportsSet = QSet<QString>(list.begin(), list.end());
        if (appSupportsSet.contains(protocolsSet) && (*it)->isShown()) {
            ++it;
        } else {
            delete *it;
            it = apps.erase(it);
        }
    }
    return apps;
}

static XdgDesktopFile *defaultApp(const QString &protocol)
{
    XdgMimeApps db;
    XdgDesktopFile *app = db.defaultApp(protocol);
    if (app && app->isValid()) {
        return app;
    } else {
        delete app; // it's fine to delete a nullptr
        return nullptr;
    }
}

static bool setDefaultApp(const QString &protocol, const XdgDesktopFile &app)
{
    XdgMimeApps db;
    return db.setDefaultApp(protocol, app);
}

XdgDesktopFile *XdgDefaultApps::emailClient()
{
    return defaultApp("x-scheme-handler/mailto"_L1);
}

QList<XdgDesktopFile *> XdgDefaultApps::emailClients()
{
    return categoryAndMimeTypeApps(u"Email"_s, QStringList() << "x-scheme-handler/mailto"_L1);
}

XdgDesktopFile *XdgDefaultApps::fileManager()
{
    return defaultApp("inode/directory"_L1);
}

QList<XdgDesktopFile *> XdgDefaultApps::fileManagers()
{
    return categoryAndMimeTypeApps("FileManager"_L1, QStringList() << "inode/directory"_L1);
}

bool XdgDefaultApps::setEmailClient(const XdgDesktopFile &app)
{
    return setDefaultApp("x-scheme-handler/mailto"_L1, app);
}

bool XdgDefaultApps::setFileManager(const XdgDesktopFile &app)
{
    return setDefaultApp("inode/directory"_L1, app);
}

bool XdgDefaultApps::setTerminal(const XdgDesktopFile &app)
{
    if (!app.isValid())
        return false;

    const QString configFile = qtxdgConfigFilename();
    QSettings settings(QSettings::UserScope, configFile);
    settings.setValue("TerminalEmulator"_L1, XdgDesktopFile::id(app.fileName()));
    return true;
}

bool XdgDefaultApps::setWebBrowser(const XdgDesktopFile &app)
{
    const QStringList protocols =
        QStringList() << getWebBrowserProtocolsGet() << getWebBrowserProtocolsSet();

    for (const QString &protocol : protocols) {
        if (!setDefaultApp(protocol, app))
            return false;
    }
    return true;
}

XdgDesktopFile *XdgDefaultApps::terminal()
{
    const QString configFile = qtxdgConfigFilename();
    QSettings settings(QSettings::UserScope, configFile);
    const QString terminalName = settings.value("TerminalEmulator"_L1, QString()).toString();
    return getTerminal(terminalName);
}

QList<XdgDesktopFile *> XdgDefaultApps::terminals()
{
    XdgMimeApps db;
    QList<XdgDesktopFile *> terminalList = db.categoryApps("TerminalEmulator"_L1);
    QList<XdgDesktopFile *>::iterator it = terminalList.begin();
    while (it != terminalList.end()) {
        if ((*it)->isShown()) {
            ++it;
        } else {
            delete *it;
            it = terminalList.erase(it);
        }
    }
    return terminalList;
}

// To be qualified as the default browser all protocols must be set to the same
// valid application
XdgDesktopFile *XdgDefaultApps::webBrowser()
{
    const QStringList webBrowserProtocolsGet = getWebBrowserProtocolsGet();
        std::vector<std::unique_ptr<XdgDesktopFile>> apps;
    for (int i = 0; i < webBrowserProtocolsGet.count(); ++i) {
        auto a = std::unique_ptr<XdgDesktopFile>(defaultApp(webBrowserProtocolsGet.at(i)));
        apps.push_back(std::move(a));
        if (apps.at(i) && apps.at(i)->isValid())
            continue;
        else
            return nullptr;
    }

    // At this point all apps are non null and valid
    for (int i = 1; i < webBrowserProtocolsGet.count(); ++i) {
        if (*apps.at(i - 1) != *apps.at(i))
            return nullptr;
    }
    return new XdgDesktopFile(*apps.at(0).get());
}

QList<XdgDesktopFile *> XdgDefaultApps::webBrowsers()
{
    return categoryAndMimeTypeApps(u"WebBrowser"_s, getWebBrowserProtocolsGet());
}
