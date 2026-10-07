#include "passsharer.h"
#include "savedcards.h"
#include "barcodecodec.h"
#include "zipfile.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <ZXing/ReadBarcode.h>

namespace {
// Stored ZIP entries suffice for JSON and already-compressed PNG assets.
QByteArray package(const QMap<QString, QByteArray> &entries)
{
    QByteArray bytes, directory;
    QDataStream out(&bytes, QIODevice::WriteOnly), central(&directory, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    central.setByteOrder(QDataStream::LittleEndian);
    for (auto it = entries.cbegin(); it != entries.cend(); ++it) {
        const QByteArray name = it.key().toUtf8(), data = it.value();
        const quint32 crc = crc32(0, reinterpret_cast<const Bytef *>(data.constData()), data.size());
        const quint32 offset = bytes.size();
        out << quint32(0x04034b50) << quint16(20) << quint16(0) << quint16(0)
            << quint16(0) << quint16(33) << crc << quint32(data.size()) << quint32(data.size())
            << quint16(name.size()) << quint16(0);
        out.writeRawData(name.constData(), name.size());
        out.writeRawData(data.constData(), data.size());
        central << quint32(0x02014b50) << quint16(20) << quint16(20) << quint16(0) << quint16(0)
                << quint16(0) << quint16(33) << crc << quint32(data.size()) << quint32(data.size())
                << quint16(name.size()) << quint16(0) << quint16(0) << quint16(0) << quint16(0)
                << quint32(0) << offset;
        central.writeRawData(name.constData(), name.size());
    }
    const quint32 offset = bytes.size();
    out.writeRawData(directory.constData(), directory.size());
    out << quint32(0x06054b50) << quint16(0) << quint16(0) << quint16(entries.size()) << quint16(entries.size())
        << quint32(directory.size()) << offset << quint16(0);
    return bytes;
}

ZXing::Barcode readCode(const QImage &image)
{
    return ZXing::ReadBarcode(ZXing::ImageView(image.constBits(), image.width(), image.height(),
                              ZXing::ImageFormat::Lum, image.bytesPerLine()),
                              ZXing::ReaderOptions().tryHarder(true));
}

QByteArray cardPackage(const QVariantMap &card)
{
    const QByteArray png = QByteArray::fromBase64(card.value("png").toByteArray());
    const QByteArray bytes = QByteArray::fromBase64(card.value("bytes").toByteArray());
    const QImage image = QImage::fromData(png, "PNG").convertToFormat(QImage::Format_Grayscale8);
    if (image.isNull() || bytes.isEmpty() || bytes.size() > 8000)
        return {};
    const auto original = readCode(image);
    const auto decoded = original.bytes();
    if (!original.isValid() || original.format() != ZXing::BarcodeFormatFromString(card.value("format").toString().toStdString())
            || QByteArray(reinterpret_cast<const char *>(decoded.data()), int(decoded.size())) != bytes)
        return {};
    const QString name = card.value("name").toString();
    QJsonObject pass{{"formatVersion", 1}, {"passTypeIdentifier", "pass.ch.p2501.savedcard"},
                     {"serialNumber", card.value("id").toString()}, {"organizationName", "Pass Viewer"},
                     {"description", name}, {"logoText", name},
                     {"backgroundColor", "rgb(255,255,255)"}, {"foregroundColor", "rgb(0,0,0)"},
                     {"labelColor", "rgb(70,70,70)"},
                     {"storeCard", QJsonObject{{"backFields", QJsonArray{
                         QJsonObject{{"key", "format"}, {"label", "Barcode format"}, {"value", card.value("format").toString()}},
                         QJsonObject{{"key", "text"}, {"label", "Barcode text"}, {"value", card.value("text").toString()}}
                     }}}},
                     {"userInfo", QJsonObject{{"passViewer", QJsonObject{{"barcodeImageVersion", 1}}}}}};
    const QMap<ZXing::BarcodeFormat, QString> formats{{ZXing::BarcodeFormat::QRCode, "QR"},
            {ZXing::BarcodeFormat::Aztec, "Aztec"}, {ZXing::BarcodeFormat::PDF417, "PDF417"},
            {ZXing::BarcodeFormat::Code128, "Code128"}};
    const QString format = formats.value(original.format());
    const QString type = format == "QR" ? "qr" : format.toLower();
    const QImage rendered = renderPassBarcode(type, bytes);
    bool standard = false;
    if (!rendered.isNull()) {
        const auto check = readCode(rendered);
        standard = !original.hasECI() && check.isValid() && check.symbologyIdentifier() == original.symbologyIdentifier();
    }
    if (standard) {
        const QJsonObject barcode{{"format", "PKBarcodeFormat" + format},
                                  {"message", QString::fromLatin1(bytes.constData(), bytes.size())},
                                  {"messageEncoding", "ISO-8859-1"}};
        pass.insert("barcode", barcode);
        pass.insert("barcodes", QJsonArray{barcode});
    }
    QVariantMap credits = card.value("icon").toMap();
    credits.remove("png");
    if (!credits.isEmpty()) {
        QJsonObject info = pass.value("userInfo").toObject();
        info.insert("iconCredits", QJsonObject::fromVariantMap(credits));
        pass.insert("userInfo", info);
        QJsonObject fields = pass.value("storeCard").toObject();
        QJsonArray back = fields.value("backFields").toArray();
        for (const QString &key : {QStringLiteral("author"), QStringLiteral("license"),
                                  QStringLiteral("sourceUrl"), QStringLiteral("licenseUrl")}) {
            if (!credits.value(key).toString().isEmpty())
                back.append(QJsonObject{{"key", "icon-" + key}, {"label", "Icon " + key},
                                        {"value", credits.value(key).toString()}});
        }
        fields.insert("backFields", back);
        pass.insert("storeCard", fields);
    }
    QMap<QString, QByteArray> files{{"barcode.png", png}};
    // Other viewers can show an image when their barcode schema cannot preserve
    // the original symbology (for example EAN or GS1). Never convert it to QR.
    if (!standard)
        files.insert("strip.png", png);
    QByteArray icon = QByteArray::fromBase64(card.value("icon").toMap().value("png").toByteArray());
    if (QImage::fromData(icon, "PNG").isNull()) {
        QImage fallback("/usr/share/icons/hicolor/128x128/apps/harbour-passviewer.png");
        if (fallback.isNull()) {
            fallback = QImage(128, 128, QImage::Format_RGB32);
            fallback.fill(QColor(40, 95, 130));
        }
        QBuffer buffer(&icon);
        buffer.open(QIODevice::WriteOnly);
        fallback.save(&buffer, "PNG");
    }
    files.insert("icon.png", icon);
    files.insert("logo.png", icon);
    files.insert("pass.json", QJsonDocument(pass).toJson(QJsonDocument::Compact));
    QJsonObject manifest;
    for (auto it = files.cbegin(); it != files.cend(); ++it)
        manifest.insert(it.key(), QString::fromLatin1(QCryptographicHash::hash(it.value(), QCryptographicHash::Sha1).toHex()));
    files.insert("manifest.json", QJsonDocument(manifest).toJson(QJsonDocument::Compact));
    // No fabricated issuer identity or signature: this is an unsigned pass.
    return package(files);
}
}

PassSharer::PassSharer(SavedCards *cards, QObject *parent) : QObject(parent), m_cards(cards) {}

QString PassSharer::prepare(const QString &path)
{
    m_error.clear();
    QByteArray data;
    QString name;
    if (path.startsWith("card:")) {
        const QVariantMap card = m_cards->card(path.mid(5));
        name = card.value("name").toString();
        try {
            if (!card.isEmpty())
                data = cardPackage(card);
        } catch (const std::exception &) {
            data.clear();
        }
    } else {
        const QUrl url(path);
        const QString local = url.isLocalFile() ? url.toLocalFile() : path;
        QFile file(local);
        ZipFile zip(local);
        if (zip.isValid() && zip.getFileList().contains("pass.json") && file.open(QIODevice::ReadOnly)
                && file.size() <= 150 * 1024 * 1024) {
            data = file.readAll();
            if (file.error() != QFile::NoError || data.size() != file.size())
                data.clear();
            name = QJsonDocument::fromJson(zip.getFile("pass.json")).object().value("description").toString();
            if (name.isEmpty())
                name = QFileInfo(local).completeBaseName();
        }
    }
    if (data.isEmpty()) {
        m_error = tr("Could not prepare this pass for sharing.");
        emit errorChanged();
        return {};
    }
    name.replace(QRegularExpression("[^\\p{L}\\p{N} _-]"), "_");
    name = name.trimmed().left(80);
    if (name.isEmpty())
        name = "Pass";
    // Immutable snapshots remain available to asynchronous sharing targets even
    // after the sheet closes, the source changes or the app exits.
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            + "/shared-passes/" + QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
    const QString target = directory + "/" + name + ".pkpass";
    QSaveFile output(target);
    if (!QDir().mkpath(directory) || !output.open(QIODevice::WriteOnly)
            || !output.setPermissions(QFile::ReadOwner | QFile::WriteOwner)
            || output.write(data) != data.size() || !output.commit()) {
        m_error = tr("Could not create the sharing file. Check available storage and try again.");
        emit errorChanged();
        return {};
    }
    emit errorChanged();
    return target;
}
