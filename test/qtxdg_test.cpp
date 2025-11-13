/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013~2015 LXQt team
 * Authors:
 *   Christian Surlykke <christian@surlykke.dk>
 *   Jerome Leclanche <jerome@leclan.ch>
 *   Luís Pereira <luis.artur.pereira@gmail.com>
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


#include "qtxdg_test.h"

#include "xdgdesktopfile.h"
#include "xdgdesktopfile_p.h"
#include "xdgdirs.h"
#include "xdgmimeapps.h"

#include <QTest>

#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include <QDebug>
#include <QSettings>

using namespace Qt::Literals::StringLiterals;

void QtXdgTest::testDefaultApp()
{
    QStringList mimedirs = XdgDirs::dataDirs();
    mimedirs.prepend(XdgDirs::dataHome(false));
    for (const QString &mimedir : std::as_const(mimedirs))
    {
        QDir dir(mimedir + u"/mime"_s);
        qDebug() << dir.path();
        QStringList filters = (QStringList() << u"*.xml"_s);
        const QFileInfoList &mediaDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &mediaDir : mediaDirs)
        {
            qDebug() << "    " << mediaDir.fileName();
            const QStringList mimeXmlFileNames = QDir(mediaDir.absoluteFilePath()).entryList(filters, QDir::Files);
            for (const QString &mimeXmlFileName : mimeXmlFileNames)
            {
                QString mimetype = mediaDir.fileName() + u'/' + mimeXmlFileName.left(mimeXmlFileName.length() - 4);
                QString xdg_utils_default = xdgUtilDefaultApp(mimetype);
                QString desktop_file_default = xdgDesktopFileDefaultApp(mimetype);

                if (xdg_utils_default != desktop_file_default)
                {
                    qDebug() << mimetype;
                    qDebug() << "xdg-mime query default:" << xdg_utils_default;
                    qDebug() << "xdgdesktopfile default:" << desktop_file_default;
                }
            }
        }

    }
}

void QtXdgTest::compare(const QString &mimetype)
{
    QString xdgUtilDefault = xdgUtilDefaultApp(mimetype);
    QString xdgDesktopDefault = xdgDesktopFileDefaultApp(mimetype);
    if (xdgUtilDefault != xdgDesktopDefault)
    {
        qDebug() << mimetype;
        qDebug() << "xdg-mime default:" << xdgUtilDefault;
        qDebug() << "xdgDesktopfile default:" << xdgDesktopDefault;
    }
}


void QtXdgTest::testTextHtml()
{
    compare(u"text/html"_s);
}

void QtXdgTest::testMeldComparison()
{
    compare(u"application/x-meld-comparison"_s);
}

void QtXdgTest::testCustomFormat()
{
    QSettings::Format desktopFormat = QSettings::registerFormat(u"list"_s, readDesktopFile, writeDesktopFile);
    QFile::remove(u"/tmp/test.list"_s);
    QFile::remove(u"/tmp/test2.list"_s);
    QSettings test(u"/tmp/test.list"_s, desktopFormat);
    test.beginGroup(u"Default Applications"_s);
    test.setValue(u"text/plain"_s, u"gvim.desktop"_s);
    test.setValue(u"text/html"_s, u"firefox.desktop"_s);
    test.endGroup();
    test.beginGroup(u"Other Applications"_s);
    test.setValue(u"application/pdf"_s, u"qpdfview.desktop"_s);
    test.setValue(u"image/svg+xml"_s, u"inkscape.desktop"_s);
    test.sync();

    QFile::copy(u"/tmp/test.list"_s, u"/tmp/test2.list"_s);

    QSettings test2(u"/tmp/test2.list"_s, desktopFormat);
    QVERIFY(test2.allKeys().size() == 4);

    test2.beginGroup(u"Default Applications"_s);
//    qDebug() << test2.value("text/plain"_s;
    QVERIFY(test2.value(u"text/plain"_s).toString() == "gvim.desktop"_L1);

//    qDebug() << test2.value("text/html");
    QVERIFY(test2.value(u"text/html"_s).toString() == "firefox.desktop"_L1);
    test2.endGroup();

    test2.beginGroup(u"Other Applications"_s);
//    qDebug() << test2.value("application/pdf");
    QVERIFY(test2.value(u"application/pdf"_s).toString() == "qpdfview.desktop"_L1);

//    qDebug() << test2.value("image/svg+xml");
    QVERIFY(test2.value(u"image/svg+xml"_s).toString() == "inkscape.desktop"_L1);
    test2.endGroup();
}


QString QtXdgTest::xdgDesktopFileDefaultApp(const QString &mimetype)
{
    XdgMimeApps appsDb;
    XdgDesktopFile *defaultApp = appsDb.defaultApp(mimetype);
    QString defaultAppS;
    if (defaultApp)
    {
        defaultAppS = QFileInfo(defaultApp->fileName()).fileName();
    }
    return defaultAppS;
}



QString QtXdgTest::xdgUtilDefaultApp(const QString &mimetype)
{
    QProcess xdg_mime;
    QString program = u"xdg-mime"_s;
    QStringList args = (QStringList() << u"query"_s << u"default"_s << mimetype);
    qDebug() << "running" << program << args.join(u' ');
    xdg_mime.start(program, args);
    xdg_mime.waitForFinished(1000);
    return QString::fromUtf8(xdg_mime.readAll()).trimmed();
}

#if 0
int main(int argc, char** args)
{
//    QtXdgTest().testDefaultApp();
//      qDebug() << "Default for text/html:" << QtXdgTest().xdgDesktopFileDefaultApp("text/html");
//    QtXdgTest().testMeldComparison();
    qDebug() << QtXdgTest().testCustomFormat();
};
#endif // 0

QTEST_MAIN(QtXdgTest)
