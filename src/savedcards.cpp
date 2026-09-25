#include "savedcards.h"

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <QUrl>

SavedCards::SavedCards(QObject *parent) : QObject(parent),
    m_directory(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/cards")
{
}

QString SavedCards::filePath(const QString &id) const
{
    if (QUuid(id).isNull() || QUuid(id).toString().mid(1, 36) != id)
        return QString();
    return m_directory + "/" + id + ".json";
}

QVariantMap SavedCards::card(const QString &id) const
{
    const QString path = filePath(id);
    if (path.isEmpty())
        return {};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 4 * 1024 * 1024)
        return {};
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();
    if (object.value("version").toInt() != 1 || object.value("id").toString() != id
            || object.value("name").toString().trimmed().isEmpty()
            || object.value("png").toString().isEmpty())
        return {};
    QVariantMap result = object.toVariantMap();
    result.insert("image", "data:image/png;base64," + result.value("png").toString());
    const QString icon = object.value("icon").toObject().value("png").toString();
    result.insert("iconImage", icon.isEmpty() ? QString() : "data:image/png;base64," + icon);
    return result;
}

QVariantList SavedCards::passes() const
{
    QVariantList result;
    const auto files = QDir(m_directory).entryInfoList({"*.json"}, QDir::Files, QDir::Name);
    for (const QFileInfo &file : files) {
        const QString id = file.completeBaseName();
        const QVariantMap value = card(id);
        if (!value.isEmpty()) {
            result.append(QVariantMap{{"name", value.value("name")}, {"path", "card:" + id},
                          {"jsondata", "{}"}, {"typeId", "saved-card"}, {"bundle", false},
                          {"updateable", false}, {"mtime", file.lastModified()}, {"iconImage", value.value("iconImage")}});
        }
    }
    return result;
}

QString SavedCards::save(const QString &name, const QVariantMap &barcode)
{
    setError(QString());
    const QString title = name.trimmed();
    const QByteArray png = QByteArray::fromBase64(barcode.value("png").toString().toLatin1());
    if (title.isEmpty() || title.size() > 200 || png.size() > 2 * 1024 * 1024
            || QImage::fromData(png, "PNG").isNull() || barcode.value("format").toString().isEmpty()) {
        setError(tr("Enter a name and scan a valid barcode before saving."));
        return QString();
    }
    if (!QDir().mkpath(m_directory)) {
        setError(tr("Could not create the saved cards folder."));
        return QString();
    }
    const QString id = QUuid::createUuid().toString().mid(1, 36);
    QJsonObject object{{"version", 1}, {"id", id}, {"name", title},
                       {"format", barcode.value("format").toString()},
                       {"text", barcode.value("text").toString()},
                       {"bytes", barcode.value("bytes").toString()},
                       {"png", QString::fromLatin1(png.toBase64())}};
    QSaveFile file(filePath(id));
    const QByteArray data = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (!file.open(QIODevice::WriteOnly) || !file.setPermissions(QFile::ReadOwner | QFile::WriteOwner)
            || file.write(data) != data.size() || !file.commit()) {
        setError(tr("Could not save the card. Check available storage and try again."));
        return QString();
    }
    emit changed();
    return id;
}

bool SavedCards::setIcon(const QString &id, const QVariantMap &icon)
{
    setError(QString());
    QVariantMap value = card(id);
    if (value.isEmpty()) {
        setError(tr("This card is no longer available."));
        return false;
    }
    value.remove("image");
    value.remove("iconImage");
    if (icon.isEmpty()) {
        value.remove("icon");
    } else {
        const QByteArray bytes = QByteArray::fromBase64(icon.value("png").toByteArray());
        QBuffer buffer;
        buffer.setData(bytes);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer, "png");
        if (bytes.size() > 256 * 1024 || reader.size() != QSize(256, 256) || reader.read().isNull()) {
            setError(tr("Please choose a valid icon."));
            return false;
        }
        QVariantMap stored;
        stored.insert("png", QString::fromLatin1(bytes.toBase64()));
        for (const QString &key : {QStringLiteral("source"), QStringLiteral("id"), QStringLiteral("name"),
                                  QStringLiteral("author"), QStringLiteral("license"),
                                  QStringLiteral("sourceUrl"), QStringLiteral("licenseUrl")}) {
            const QString text = icon.value(key).toString().left(1024);
            if (key.endsWith("Url") && QUrl(text).scheme() != "https")
                continue;
            stored.insert(key, text);
        }
        stored.insert("brand", icon.value("brand").toBool());
        value.insert("icon", stored);
    }
    QSaveFile file(filePath(id));
    const QByteArray data = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
    if (!file.open(QIODevice::WriteOnly) || !file.setPermissions(QFile::ReadOwner | QFile::WriteOwner)
            || file.write(data) != data.size() || !file.commit()) {
        setError(tr("Could not save the icon. Check available storage and try again."));
        return false;
    }
    emit changed();
    return true;
}

bool SavedCards::remove(const QString &id)
{
    setError(QString());
    const QString path = filePath(id);
    if (path.isEmpty() || !QFile::remove(path)) {
        setError(tr("Could not delete the saved card."));
        return false;
    }
    emit changed();
    return true;
}

void SavedCards::setError(const QString &error)
{
    m_error = error;
    emit errorChanged();
}
