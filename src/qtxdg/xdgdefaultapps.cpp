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
#include "xdgmimeapps.h"

#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

static QStringList getWebBrowserProtocolsGet()
{
    // Protocols needed to quailify a application as the default browser
    static const QStringList webBrowserProtocolsGet = {
        QL1S("text/html"),
        QL1S("x-scheme-handler/http"),
        QL1S("x-scheme-handler/https")
    };
    return webBrowserProtocolsGet;
}

static QStringList getWebBrowserProtocolsSet()
{
    // When setting an application as the default browser xdg-settings also
    // sets these protocols. We simply follow it as good practice.
    static const QStringList webBrowserProtocolsSet {
        QL1S("x-scheme-handler/about"),
        QL1S("x-scheme-handler/unknown")
    };
    return webBrowserProtocolsSet;
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
