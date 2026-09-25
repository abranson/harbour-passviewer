#include <QCoreApplication>
#include <QCryptographicHash>
#include <QSettings>
#include <QStandardPaths>
#include "settingsstore.h"

SettingsStore::SettingsStore(QObject *parent) :
    QObject(parent),
    m_settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/" + QCoreApplication::applicationName() + ".conf",
             QSettings::NativeFormat)
{
    if (!m_settings.contains("timeline/archive_after_hours")) {
        // Migrate once, then keep archiving independent of the highlight window.
        const int delay = m_settings.value("timeline/archive_past", true).toBool() ? hoursAfter() : -1;
        m_settings.setValue("timeline/archive_after_hours", delay);
    }
}

int SettingsStore::archiveState(const QString &key) {
    const QString digest = QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex());
    return m_settings.value("timeline/items/" + digest, 0).toInt();
}

bool SettingsStore::setArchiveState(const QString &key, int state) {
    if (key.isEmpty() || state < 0 || state > 2)
        return false;
    // 0 follows dates, 1 is manually archived, 2 is explicitly kept in the list.
    const QString digest = QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex());
    const QString setting = "timeline/items/" + digest;
    const QVariant previous = m_settings.value(setting);
    m_settings.setValue(setting, state);
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError) {
        if (previous.isValid())
            m_settings.setValue(setting, previous);
        else
            m_settings.remove(setting);
        return false;
    }
    emit archiveStateChanged();
    return true;
}

int SettingsStore::archiveAfterHours() {
    return m_settings.value("timeline/archive_after_hours", 4).toInt();
}

void SettingsStore::setArchiveAfterHours(int value) {
    if (value < -1 || value > 720)
        return;
    int oldValue = archiveAfterHours();
    m_settings.setValue("timeline/archive_after_hours", value);
    if (value != oldValue)
        emit archiveAfterHoursChanged();
}

int SettingsStore::sortBy() {
    return m_settings.value("sort/sort_by", 0).toInt();
}

void SettingsStore::setSortBy(int value) {
    int oldValue = sortBy();
    m_settings.setValue("sort/sort_by", value);
    if (value != oldValue)
        emit sortByChanged();
}

bool SettingsStore::checkTime() {
    return m_settings.value("time/check", true).toBool();
}

void SettingsStore::setCheckTime(bool value) {
    bool oldValue = checkTime();
    m_settings.setValue("time/check", value);
    if (value != oldValue)
        emit checkTimeChanged();
}

int SettingsStore::hoursBefore() {
    return m_settings.value("time/hours_before", 4).toInt();
}

void SettingsStore::setHoursBefore(int value) {
    int oldValue = hoursBefore();
    m_settings.setValue("time/hours_before", value);
    if (value != oldValue)
        emit hoursBeforeChanged();
}

int SettingsStore::hoursAfter() {
    return m_settings.value("time/hours_after", 4).toInt();
}

void SettingsStore::setHoursAfter(int value) {
    int oldValue = hoursAfter();
    m_settings.setValue("time/hours_after", value);
    if (value != oldValue)
        emit hoursAfterChanged();
}

bool SettingsStore::checkDistance() {
    return m_settings.value("distance/check", false).toBool();
}

void SettingsStore::setCheckDistance(bool value) {
    bool oldValue = checkDistance();
    m_settings.setValue("distance/check", value);
    if (value != oldValue)
        emit checkDistanceChanged();
}

bool SettingsStore::useHere() {
    return m_settings.value("distance/use_here", false).toBool();
}

void SettingsStore::setUseHere(bool value) {
    bool oldValue = useHere();
    m_settings.setValue("distance/use_here", value);
    if (value != oldValue)
        emit useHereChanged();
}

int SettingsStore::maxDistance() {
    return m_settings.value("distance/max", 1000).toInt();
}

void SettingsStore::setMaxDistance(int value) {
    int oldValue = maxDistance();
    m_settings.setValue("distance/max", value);
    if (value != oldValue)
        emit maxDistanceChanged();
}

bool SettingsStore::overrideDistance() {
    return m_settings.value("distance/override", true).toBool();
}

void SettingsStore::setOverrideDistance(bool value) {
    bool oldValue = overrideDistance();
    m_settings.setValue("distance/override", value);
    if (value != oldValue)
        emit overrideDistanceChanged();
}

bool SettingsStore::barcodeTap() {
    return m_settings.value("barcode/tap", false).toBool();
}

void SettingsStore::setBarcodeTap(bool value) {
    bool oldValue = barcodeTap();
    m_settings.setValue("barcode/tap", value);
    if (value != oldValue)
        emit barcodeTapChanged();
}
