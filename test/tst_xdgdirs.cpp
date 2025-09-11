/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
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

#include <QObject>

#include "xdgdirs.h"

#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QTest>
#include <QTemporaryDir>

using namespace Qt::Literals::StringLiterals;

QString randomString(int length)
{
    static const QString characterPool("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"_L1);
    if (length < 0)
        return QString();

    QString randomString;
    for (int i = 0; i < length; ++i) {
        const int index = QRandomGenerator::global()->generate() % characterPool.size();
        randomString.append(characterPool.at(index));
    }
    return randomString;
}

class tst_xdgdirs : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testDataHome();
    void testConfigHome();
    void testDataDirs();
    void testConfigDirs();
    void testCacheHome();
    void testAutostartHome();
    void testAutostartDirs();
    void testNonWritableLocations();

private:
    void setDefaultLocations();
    void setCustomLocations();
    void setNonWritableLocations();

    QString noSlashPostfix;

    QString m_configHome;
    QTemporaryDir m_configHomeTemp;
    QString m_configDirs;
    QTemporaryDir m_configDirsTemp;
    QString m_dataHome;
    QTemporaryDir m_dataHomeTemp;
    QString m_dataDirs;
    QTemporaryDir m_dataDirsTemp;
    QString m_cacheHome;
    QTemporaryDir m_cacheHomeTemp;

    QDir m_nonWritableRoot;
    QString m_configHomeNonWritable;
};

void tst_xdgdirs::initTestCase()
{
    QCoreApplication::instance()->setOrganizationName(QStringLiteral("QtXdg"));
    QCoreApplication::instance()->setApplicationName(QStringLiteral("tst_xdgdirs"));

    noSlashPostfix = QCoreApplication::applicationName();
}

void tst_xdgdirs::cleanupTestCase()
{
    QCoreApplication::instance()->setOrganizationName(QString());
    QCoreApplication::instance()->setApplicationName(QString());

    QFile f(m_nonWritableRoot.path());
    QVERIFY(f.setPermissions(QFile::WriteUser | QFile::WriteOwner));
    QVERIFY(m_nonWritableRoot.removeRecursively());
}

void tst_xdgdirs::setDefaultLocations()
{
    qputenv("XDG_CONFIG_HOME", QByteArray());
    qputenv("XDG_CONFIG_DIRS", QByteArray());
    qputenv("XDG_DATA_HOME", QByteArray());
    qputenv("XDG_DATA_DIRS", QByteArray());
    qputenv("XDG_CACHE_HOME", QByteArray());
}

void tst_xdgdirs::setCustomLocations()
{
    m_configHome = m_configHomeTemp.path();
    m_configDirs = m_configDirsTemp.path();
    m_dataHome = m_dataHomeTemp.path();
    m_dataDirs = m_dataDirsTemp.path();
    m_cacheHome = m_cacheHomeTemp.path();
    qputenv("XDG_CONFIG_HOME", QFile::encodeName(m_configHome));
    qputenv("XDG_CONFIG_DIRS", QFile::encodeName(m_configDirs));
    qputenv("XDG_DATA_HOME", QFile::encodeName(m_dataHome));
    qputenv("XDG_DATA_DIRS", QFile::encodeName(m_dataDirs));
    qputenv("XDG_CACHE_HOME", QFile::encodeName(m_cacheHome));

}

void tst_xdgdirs::setNonWritableLocations()
{
    const QString randomDir = randomString(8);
    m_nonWritableRoot = QDir::temp();

    QVERIFY(m_nonWritableRoot.mkdir(randomDir, QFileDevice::ReadUser));
    m_nonWritableRoot.cd(randomDir);
    m_configHomeNonWritable = m_nonWritableRoot.path() + u'/' + "nonwritabledir"_L1;

    qputenv("XDG_CONFIG_HOME", QFile::encodeName(m_configHomeNonWritable));
    qputenv("XDG_DATA_HOME", QFile::encodeName(m_configHomeNonWritable));
    qputenv("XDG_CACHE_HOME", QFile::encodeName(m_configHomeNonWritable));
}

void tst_xdgdirs::testDataHome()
{
    setDefaultLocations();
    const QString expectedDataHome = QDir::homePath() + QString::fromLatin1("/.local/share");
    QCOMPARE(XdgDirs::dataHome(), expectedDataHome);
    QCOMPARE(XdgDirs::dataHome(false), expectedDataHome);
    QCOMPARE(XdgDirs::dataHome(true), expectedDataHome);

    setCustomLocations();
    QCOMPARE(XdgDirs::dataHome(), m_dataHome);
    QCOMPARE(XdgDirs::dataHome(false), m_dataHome);
}

void tst_xdgdirs::testConfigHome()
{
    setDefaultLocations();
    const QString expectedConfigHome = QDir::homePath() + QString::fromLatin1("/.config");
    QCOMPARE(XdgDirs::configHome(), expectedConfigHome);
    QCOMPARE(XdgDirs::configHome(false), expectedConfigHome);

    setCustomLocations();
    QCOMPARE(XdgDirs::configHome(), m_configHome);
    QCOMPARE(XdgDirs::configHome(false), m_configHome);
}

void tst_xdgdirs::testDataDirs()
{
    const QString postfix = QString::fromLatin1("/") + QCoreApplication::applicationName();

    setDefaultLocations();
    const QStringList dataDirs = XdgDirs::dataDirs();
    QCOMPARE(dataDirs.count(), 2);
    QCOMPARE(dataDirs.at(0), QString::fromLatin1("/usr/local/share"));
    QCOMPARE(dataDirs.at(1), QString::fromLatin1("/usr/share"));

    const QStringList dataDirsWithPostfix = XdgDirs::dataDirs(postfix);
    QCOMPARE(dataDirsWithPostfix.count(), 2);
    QCOMPARE(dataDirsWithPostfix.at(0), QString::fromLatin1("/usr/local/share") + postfix);
    QCOMPARE(dataDirsWithPostfix.at(1), QString::fromLatin1("/usr/share") + postfix);

    const QStringList dataDirsWithNoSlashPostfix = XdgDirs::dataDirs(noSlashPostfix);
    QCOMPARE(dataDirsWithNoSlashPostfix.count(), 2);
    QCOMPARE(dataDirsWithNoSlashPostfix.at(0), "/usr/local/share"_L1 + u'/' + noSlashPostfix);
    QCOMPARE(dataDirsWithNoSlashPostfix.at(1), "/usr/share"_L1 + u'/' + noSlashPostfix);

    setCustomLocations();
    const QStringList dataDirsCustom = XdgDirs::dataDirs();
    QCOMPARE(dataDirsCustom.count(), 1);
    QCOMPARE(dataDirsCustom.at(0), m_dataDirs);

    const QStringList dataDirsCustomWithPostfix = XdgDirs::dataDirs(postfix);
    QCOMPARE(dataDirsCustomWithPostfix.count(), 1);
    QCOMPARE(dataDirsCustomWithPostfix.at(0), m_dataDirs + postfix);

    const QStringList dataDirsCustomWithNoSlashPostfix;
    QCOMPARE(dataDirsCustomWithPostfix.count(), 1);
    QCOMPARE(dataDirsCustomWithPostfix.at(0), m_dataDirs + u'/' + noSlashPostfix);
}

void tst_xdgdirs::testConfigDirs()
{
    const QString postfix = QString::fromLatin1("/") + QCoreApplication::applicationName();
    setDefaultLocations();

    const QStringList configDirs = XdgDirs::configDirs();
    QCOMPARE(configDirs.count(), 1);
    QCOMPARE(configDirs.at(0), QString::fromLatin1("/etc/xdg"));

    const QStringList configDirsWithPostfix = XdgDirs::configDirs(postfix);
    QCOMPARE(configDirsWithPostfix.count(), 1);
    QCOMPARE(configDirsWithPostfix.at(0), QString::fromLatin1("/etc/xdg") + postfix);

    setCustomLocations();
    const QStringList configDirsCustom = XdgDirs::configDirs();
    QCOMPARE(configDirsCustom.count(), 1);
    QCOMPARE(configDirsCustom.at(0), m_configDirs);

    const QStringList configDirsCustomWithPostfix = XdgDirs::configDirs(postfix);
    QCOMPARE(configDirsCustomWithPostfix.count(), 1);
    QCOMPARE(configDirsCustomWithPostfix.at(0), m_configDirs + postfix);
}

void tst_xdgdirs::testCacheHome()
{
    setDefaultLocations();
    const QString expectedCacheHome = QDir::homePath() + QStringLiteral("/.cache");
    QCOMPARE(XdgDirs::cacheHome(), expectedCacheHome);
    QCOMPARE(XdgDirs::cacheHome(false), expectedCacheHome);

    setCustomLocations();
    const QString expectedCacheHomeCustom = XdgDirs::cacheHome();
    QCOMPARE(XdgDirs::cacheHome(), expectedCacheHomeCustom);
    QCOMPARE(XdgDirs::cacheHome(false), expectedCacheHomeCustom);

}

void tst_xdgdirs::testAutostartHome()
{
    setDefaultLocations();
    const QString expectedAutoStartHome = QDir::homePath() + QString::fromLatin1("/.config/autostart");
    QCOMPARE(XdgDirs::autostartHome(), expectedAutoStartHome);
    QCOMPARE(XdgDirs::autostartHome(false), expectedAutoStartHome);

    setCustomLocations();
    const QString expectedAutostartHomeCustom = XdgDirs::configHome() + QString::fromLatin1("/autostart");
    QCOMPARE(XdgDirs::autostartHome(), expectedAutostartHomeCustom);
    QCOMPARE(XdgDirs::autostartHome(false), expectedAutostartHomeCustom);
}

void tst_xdgdirs::testAutostartDirs()
{
    const QString postfix = QString::fromLatin1("/") + QCoreApplication::applicationName();

    setDefaultLocations();
    const QStringList autostartDirs = XdgDirs::autostartDirs();
    QCOMPARE(autostartDirs.count(), 1);
    QCOMPARE(autostartDirs.at(0), QString::fromLatin1("/etc/xdg/autostart"));

    const QStringList autostartDirsWithPostfix = XdgDirs::autostartDirs(postfix);
    QCOMPARE(autostartDirsWithPostfix.count(), 1);
    QCOMPARE(autostartDirsWithPostfix.at(0), QString::fromLatin1("/etc/xdg/autostart") + postfix);

    const QStringList autostartDirsWithNoSlashPostfix = XdgDirs::autostartDirs(noSlashPostfix);
    QCOMPARE(autostartDirsWithNoSlashPostfix.count(), 1);
    QCOMPARE(autostartDirsWithNoSlashPostfix.at(0), "/etc/xdg/autostart"_L1 + u'/' + noSlashPostfix);

    setCustomLocations();
    const QStringList autostartDirsCustom = XdgDirs::autostartDirs();
    QCOMPARE(autostartDirsCustom.count(), 1);
    QCOMPARE(autostartDirsCustom.at(0), m_configDirs + QString::fromLatin1("/autostart"));

    const QStringList autostartDirsCustomWithPostfix = XdgDirs::autostartDirs(postfix);
    QCOMPARE(autostartDirsCustomWithPostfix.count(), 1);
    QCOMPARE(autostartDirsCustomWithPostfix.at(0), m_configDirs + QString::fromLatin1("/autostart") + postfix);

    const QStringList autostartDirsCustomWithNoSlashPostfix = XdgDirs::autostartDirs(noSlashPostfix);
    QCOMPARE(autostartDirsCustomWithNoSlashPostfix.count(), 1);
    QCOMPARE(autostartDirsCustomWithNoSlashPostfix.at(0), m_configDirs + "/autostart"_L1 + u'/' + noSlashPostfix);
}

void tst_xdgdirs::testNonWritableLocations()
{
    setNonWritableLocations();
    QCOMPARE(XdgDirs::configHome(true), QString());
    QCOMPARE(XdgDirs::dataHome(true), QString());
    QCOMPARE(XdgDirs::cacheHome(true), QString());
    QCOMPARE(XdgDirs::autostartHome(true), QString());
}

QTEST_APPLESS_MAIN(tst_xdgdirs)
#include "tst_xdgdirs.moc"
