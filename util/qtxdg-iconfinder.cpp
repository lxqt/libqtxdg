/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2017  Luís Pereira <luis.artur.pereira@gmail.com>
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

#include <QGuiApplication> // XdgIconLoader needs a QGuiApplication
#include <QCommandLineParser>
#include <private/xdgiconloader/xdgiconloader_p.h>

#include <iostream>
#include <QDebug>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("qtxdg-iconfinder"));
    app.setApplicationVersion(QStringLiteral(QTXDG_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("QtXdg icon finder"));
    parser.addPositionalArgument(QStringLiteral("iconnames"),
        QStringLiteral("The icon names to search for"),
        QStringLiteral("[iconnames...]"));
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    if (parser.positionalArguments().isEmpty())
        parser.showHelp(EXIT_FAILURE);

    const auto icons = parser.positionalArguments();
    for (const QString& iconName : icons) {
        const auto info = XdgIconLoader::instance()->loadIcon(iconName);
        const auto icon = info.iconName;
        const auto entries = info.entries;

        std::cout << qPrintable(iconName) <<
            qPrintable(QString::fromLatin1(":")) << qPrintable(icon) << "\n";

        for (const auto &i : entries) {
            std::cout << "\t" << qPrintable(i->filename) << "\n";
        }
    }
    return EXIT_SUCCESS;
}
