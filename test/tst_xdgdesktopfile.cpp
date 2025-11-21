/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs.
 * Copyright (C) 2016  Luís Pereira <luis.artur.pereira@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "tst_xdgdesktopfile.h"
#include "XdgDesktopFile"

#include <QString>
#include <QTemporaryFile>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

class Language
{
public:
    Language (const QString& lang)
    : mPreviousLang(QString::fromLocal8Bit(qgetenv("LC_MESSAGES")))
    {
        qputenv("LC_MESSAGES", lang.toLocal8Bit());
    }
    ~Language()
    {
        qputenv("LC_MESSAGES", mPreviousLang.toLocal8Bit());
    }
private:
    QString mPreviousLang;
};


QTEST_MAIN(tst_xdgdesktopfile)

void tst_xdgdesktopfile::testRead()
{
    QTemporaryFile file(QDir::temp().filePath(u"testReadXXXXXX.desktop"_s));
    QVERIFY(file.open());
    const QString fileName = file.fileName();
    QTextStream ts(&file);
    ts <<
       "[Desktop Entry]\n"
       "Type=Application\n"
       "Name=MyApp\n"
       "Icon=foobar\n"
       "MimeType=text/plain;image/png;;\n"
       "\n";
    file.close();
    QVERIFY(QFile::exists(fileName));

    XdgDesktopFile df;
    df.load(fileName);

    QVERIFY(df.isValid());
    QCOMPARE(df.type(), XdgDesktopFile::ApplicationType);
    QCOMPARE(df.name(), "MyApp"_L1);
    QCOMPARE(df.iconName(), "foobar"_L1);
    QCOMPARE(df.mimeTypes(), QStringList() << "text/plain"_L1
             << "image/png"_L1);
    QCOMPARE(df.fileName(), QFileInfo(fileName).canonicalFilePath());
}

void tst_xdgdesktopfile::testReadLocalized_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("translation");

    const QString pt = QString::fromUtf8("A Minha Aplicação");
    const QString pt_BR = QString::fromUtf8("O Meu Aplicativo");

    QTest::newRow("pt") << u"pt"_s << pt;
    QTest::newRow("pt_PT") << u"pt_PT"_s  << pt;
    QTest::newRow("pt_BR") << u"pt_BR"_s << pt_BR;

    QTest::newRow("de") << u"de"_s << u"My Application"_s;
}

void tst_xdgdesktopfile::testReadLocalized()
{
    QTemporaryFile file(QDir::temp().filePath(u"testReadLocalizedXXXXXX.desktop"_s));
    QVERIFY(file.open());
    const QString fileName = file.fileName();
    QTextStream ts(&file);
    ts << QString::fromUtf8(
    "[Desktop Entry]\n"
    "Type=Application\n"
    "Name=My Application\n"
    "Name[pt]=A Minha Aplicação\n"
    "Name[pt_BR]=O Meu Aplicativo\n"
    "Icon=foo\n"
    "\n");
    file.close();

    QVERIFY(QFile::exists(fileName));

    XdgDesktopFile df;
    df.load(fileName);
    QVERIFY(df.isValid());

    QFETCH(QString, locale);
    QFETCH(QString, translation);

    Language lang(locale);

    QCOMPARE(df.localizedValue(u"Name"_s).toString(), translation);
}
