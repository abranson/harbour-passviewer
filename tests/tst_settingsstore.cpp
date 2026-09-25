#include "../src/settingsstore.h"
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class SettingsStoreTest : public QObject
{
    Q_OBJECT
private slots:
    void archiveDelayMigration_data() {
        QTest::addColumn<bool>("enabled");
        QTest::newRow("enabled") << true;
        QTest::newRow("never") << false;
    }

    void archiveDelayMigration() {
        QFETCH(bool, enabled);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        qputenv("XDG_CONFIG_HOME", directory.path().toUtf8());
        QCoreApplication::setOrganizationName("PassViewerTests");
        QCoreApplication::setApplicationName("Migration");
        QSettings previous(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                           + "/Migration.conf", QSettings::NativeFormat);
        previous.setValue("timeline/archive_past", enabled);
        previous.setValue("time/hours_after", 7);
        previous.sync();
        SettingsStore store;
        QCOMPARE(store.archiveAfterHours(), enabled ? 7 : -1);
        store.setHoursAfter(2);
        QCOMPARE(store.archiveAfterHours(), enabled ? 7 : -1);
        store.setArchiveAfterHours(24);
        SettingsStore reopened;
        QCOMPARE(reopened.archiveAfterHours(), 24);
        reopened.setArchiveAfterHours(-1);
        SettingsStore never;
        QCOMPARE(never.archiveAfterHours(), -1);
        never.setArchiveAfterHours(721);
        QCOMPARE(never.archiveAfterHours(), -1);
    }

    void archivePersistence() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        qputenv("XDG_CONFIG_HOME", directory.path().toUtf8());
        QCoreApplication::setOrganizationName("PassViewerTests");
        QCoreApplication::setApplicationName("Archive");
        SettingsStore store;
        QSignalSpy changed(&store, &SettingsStore::archiveStateChanged);
        QCOMPARE(store.archiveState("card:one"), 0);
        QVERIFY(store.setArchiveState("card:one", 1));
        QCOMPARE(changed.count(), 1);
        SettingsStore reopened;
        QCOMPARE(reopened.archiveState("card:one"), 1);
        QCOMPARE(reopened.archiveState("card:two"), 0);
        QVERIFY(reopened.setArchiveState("card:one", 2));
        SettingsStore restored;
        QCOMPARE(restored.archiveState("card:one"), 2);
        QVERIFY(!restored.setArchiveState("", 1));
        QVERIFY(!restored.setArchiveState("card:one", 3));
        QCOMPARE(restored.archiveState("card:one"), 2);
        QVERIFY(restored.setArchiveState("card:one", 0));
        SettingsStore automatic;
        QCOMPARE(automatic.archiveState("card:one"), 0);
    }
};
QTEST_GUILESS_MAIN(SettingsStoreTest)
#include "tst_settingsstore.moc"
