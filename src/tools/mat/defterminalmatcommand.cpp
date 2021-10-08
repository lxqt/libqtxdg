/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2021  Luís Pereira <luis.artur.pereira@gmail.com>
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

#include "defterminalmatcommand.h"

#include "matglobals.h"
#include "xdgmacros.h"
#include "xdgdefaultapps.h"
#include "xdgdesktopfile.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include <iostream>

enum DefTerminalCommandMode {
    CommandModeGetDefTerminal,
    CommandModeSetDefTerminal,
    CommandModeListAvailableTerminals,
};

struct DefTerminalData {
    DefTerminalData() : mode(CommandModeGetDefTerminal) {}

    DefTerminalCommandMode mode;
    QString defTerminalName;
};

static CommandLineParseResult parseCommandLine(QCommandLineParser *parser, DefTerminalData *data, QString *errorMessage)
{
    parser->clearPositionalArguments();
    parser->setApplicationDescription(QL1S("Get/Set the default terminal"));

    parser->addPositionalArgument(QL1S("def-terminal"), QL1S());

    const QCommandLineOption defTerminalNameOption(QStringList() << QSL("s") << QSL("set"),
                QSL("Terminal to be set as default"), QSL("terminal"));

    const QCommandLineOption listAvailableOption(QStringList() << QSL("l") << QSL("list-available"),
                QSL("List available terminals"));

    parser->addOption(defTerminalNameOption);
    parser->addOption(listAvailableOption);
    const QCommandLineOption helpOption = parser->addHelpOption();
    const QCommandLineOption versionOption = parser->addVersionOption();

    if (!parser->parse(QCoreApplication::arguments())) {
        *errorMessage = parser->errorText();
        return CommandLineError;
    }

    if (parser->isSet(versionOption)) {
        return CommandLineVersionRequested;
    }

    if (parser->isSet(helpOption)) {
        return CommandLineHelpRequested;
    }

    const bool isListAvailableSet = parser->isSet(listAvailableOption);
    const bool isDefTerminalNameSet = parser->isSet(defTerminalNameOption);
    QString defTerminalName;

    if (isDefTerminalNameSet)
        defTerminalName = parser->value(defTerminalNameOption);

    QStringList posArgs = parser->positionalArguments();
    posArgs.removeAt(0);

    if (isDefTerminalNameSet && !posArgs.empty()) {
        *errorMessage = QSL("Extra arguments given: ");
        errorMessage->append(posArgs.join(QLatin1Char(',')));
        return CommandLineError;
    }

    if (!isDefTerminalNameSet && !posArgs.empty()) {
        *errorMessage = QSL("To set the default terminal use the -s/--set option");
        return CommandLineError;
    }

    if (isListAvailableSet && (isDefTerminalNameSet || !posArgs.empty())) {
        *errorMessage = QSL("list-available can't be used with other options and doesn't take arguments");
        return CommandLineError;
    }

    if (isListAvailableSet) {
        data->mode = CommandModeListAvailableTerminals;
    } else {
        data->mode = isDefTerminalNameSet ? CommandModeSetDefTerminal: CommandModeGetDefTerminal;
        data->defTerminalName = defTerminalName;
    }

    return CommandLineOk;
}

DefTerminalMatCommand::DefTerminalMatCommand(QCommandLineParser *parser)
    : MatCommandInterface(QL1S("def-terminal"),
                          QSL("Get/Set the default terminal"),
                          parser)
{
   Q_CHECK_PTR(parser);
}

DefTerminalMatCommand::~DefTerminalMatCommand() = default;

int DefTerminalMatCommand::run(const QStringList & /*arguments*/)
{
    bool success = true;
    DefTerminalData data;
    QString errorMessage;
    if (!MatCommandInterface::parser()) {
        qFatal("DefTerminalMatCommand::run: MatCommandInterface::parser() returned a null pointer");
    }
    switch(parseCommandLine(parser(), &data, &errorMessage)) {
    case CommandLineOk:
        break;
    case CommandLineError:
        std::cerr << qPrintable(errorMessage);
        std::cerr << "\n\n";
        std::cerr << qPrintable(parser()->helpText());
        return EXIT_FAILURE;
    case CommandLineVersionRequested:
        showVersion();
        Q_UNREACHABLE();
    case CommandLineHelpRequested:
        showHelp();
        Q_UNREACHABLE();
    }

    if (data.mode == CommandModeListAvailableTerminals) {
        const auto terminals = XdgDefaultApps::terminals();
        for (const auto *terminal : terminals) {
            QFileInfo fi{terminal->fileName()};
            std::cout << qPrintable(fi.fileName()) << "\n";
        }

        qDeleteAll(terminals);
        return EXIT_SUCCESS;
    }

    if (data.mode == CommandModeGetDefTerminal) { // Get default terminal
        XdgDesktopFile *defTerminal = XdgDefaultApps::terminal();
        if (defTerminal != nullptr && defTerminal->isValid()) {
            QFileInfo f(defTerminal->fileName());
            std::cout << qPrintable(f.fileName()) << "\n";
            delete defTerminal;
        }
    } else { // Set default terminal
        XdgDesktopFile toSetDefTerminal;
        if (toSetDefTerminal.load(data.defTerminalName)) {
            if (XdgDefaultApps::setTerminal(toSetDefTerminal)) {
                std::cout << qPrintable(QSL("Set '%1' as the default terminal\n").arg(toSetDefTerminal.fileName()));
            } else {
                std::cerr << qPrintable(QSL("Could not set '%1' as the default terminal\n").arg(toSetDefTerminal.fileName()));
                success = false;
            }
        } else { // could not load application file
            std::cerr << qPrintable(QSL("Could not find find '%1'\n").arg(data.defTerminalName));
            success = false;
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

