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
#include <QElapsedTimer>
#include <private/xdgiconloader/xdgiconloader_p.h>


#include <iostream>
#include <QDebug>

using namespace Qt::Literals::StringLiterals;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(u"qtxdg-iconfinder"_s);
    app.setApplicationVersion(QStringLiteral(QTXDG_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(u"QtXdg icon finder"_s);
    parser.addPositionalArgument(u"iconnames"_s,
        u"The icon names to search for"_s,
        u"[iconnames...]"_s);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    if (parser.positionalArguments().isEmpty())
        parser.showHelp(EXIT_FAILURE);

    qint64 totalElapsed = 0;
    const auto icons = parser.positionalArguments();
    for (const QString& iconName : icons) {
        QElapsedTimer t;
        t.start();
        const auto info = XdgIconLoader::instance()->loadIcon(iconName);
        qint64 elapsed = t.elapsed();
        const auto icon = info.iconName;
        const auto &entries = info.entries;

        std::cout << qPrintable(iconName) <<
            qPrintable(":"_L1) << qPrintable(icon) <<
            qPrintable(":"_L1) <<
            qPrintable(QString::number(elapsed)) << "\n";

        for (const auto &i : entries) {
            std::cout << "\t" << qPrintable(i->filename) << "\n";
        }
        totalElapsed += elapsed;
    }

    std::cout << qPrintable("Total loadIcon() time: "_L1) <<
        qPrintable(QString::number(totalElapsed)) <<
        qPrintable(" ms"_L1) << "\n";

    return EXIT_SUCCESS;
}
