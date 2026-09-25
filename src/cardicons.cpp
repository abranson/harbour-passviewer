#include "cardicons.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPainter>
#include <QRegularExpression>
#include <QTimer>
#include <QUrlQuery>
#include <cmath>

namespace {
QString pngText(const QImage &image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    return image.save(&buffer, "PNG") ? QString::fromLatin1(bytes.toBase64()) : QString();
}

QUrl iconUrl(const QString &id)
{
    QString path = id;
    path.replace(':', '/');
    return QUrl("https://api.iconify.design/" + path + ".svg?color=%23222222&height=256");
}
}

CardIcons::CardIcons(QObject *parent) : QObject(parent)
{
}

CardIcons::~CardIcons()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
    }
}

void CardIcons::cancel()
{
    if (m_reply) {
        QNetworkReply *reply = m_reply;
        m_reply.clear();
        reply->abort();
        reply->deleteLater();
        emit busyChanged();
    }
}

void CardIcons::setError(const QString &error)
{
    m_error = error;
    emit errorChanged();
}

void CardIcons::search(const QString &query)
{
    cancel();
    setError(QString());
    m_results.clear();
    emit resultsChanged();
    const QString text = query.trimmed().left(100);
    if (text.isEmpty())
        return;
    QUrl url("https://api.iconify.design/search");
    QUrlQuery parameters;
    parameters.addQueryItem("query", text);
    parameters.addQueryItem("prefixes", "simple-icons,mdi");
    parameters.addQueryItem("limit", "32");
    url.setQuery(parameters);
    request(url);
}

void CardIcons::selectCatalog(const QString &id)
{
    // Only fetch icons returned by this search, never an arbitrary URL.
    for (const QVariant &value : m_results) {
        const QVariantMap result = value.toMap();
        if (result.value("id").toString() == id) {
            cancel();
            setError(QString());
            request(iconUrl(id), result);
            return;
        }
    }
}

void CardIcons::request(const QUrl &url, const QVariantMap &credit)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "PassViewer/2.0 (Sailfish OS)");
    QNetworkReply *reply = m_network.get(request);
    m_reply = reply;
    emit busyChanged();
    // Qt 5.6 has no transfer timeout API. Bound both time and response size.
    QTimer *timeout = new QTimer(reply);
    timeout->setSingleShot(true);
    timeout->setInterval(15000);
    connect(timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
    timeout->start();
    connect(reply, &QIODevice::readyRead, this, [reply]() {
        if (reply->bytesAvailable() > 1024 * 1024)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, credit]() {
        reply->deleteLater();
        if (m_reply != reply)
            return;
        m_reply.clear();
        emit busyChanged();
        if (reply->error() != QNetworkReply::NoError
                || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200
                || reply->bytesAvailable() > 1024 * 1024) {
            setError(tr("Could not load the icon catalog. Check your connection and try again."));
            return;
        }
        const QByteArray bytes = reply->readAll();
        if (!credit.isEmpty()) {
            QBuffer buffer;
            buffer.setData(bytes);
            buffer.open(QIODevice::ReadOnly);
            QImageReader reader(&buffer, "svg");
            const QSize size = reader.size();
            if (!size.isValid() || size.width() > 4096 || size.height() > 4096) {
                setError(tr("This icon could not be opened."));
                return;
            }
            reader.setScaledSize(size.scaled(512, 512, Qt::KeepAspectRatio));
            const QImage image = reader.read();
            if (image.isNull())
                setError(tr("This icon could not be opened."));
            else
                setImage(image, credit);
            return;
        }
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
        const QJsonObject object = document.object();
        if (error.error != QJsonParseError::NoError || !object.value("icons").isArray()) {
            setError(tr("The icon catalog returned an invalid response."));
            return;
        }
        const QJsonObject collections = object.value("collections").toObject();
        static const QRegularExpression validId("^(simple-icons|mdi):[a-z0-9]+(?:-[a-z0-9]+)*$");
        for (const QJsonValue &value : object.value("icons").toArray()) {
            const QString id = value.toString();
            if (!validId.match(id).hasMatch())
                continue;
            const QString prefix = id.section(':', 0, 0);
            const QString name = id.section(':', 1).replace('-', ' ');
            const QJsonObject info = collections.value(prefix).toObject();
            const QUrl licenseUrl(info.value("license").toObject().value("url").toString());
            m_results.append(QVariantMap{{"id", id}, {"name", name}, {"source", "Iconify"},
                {"preview", iconUrl(id).toString()},
                {"sourceUrl", "https://icon-sets.iconify.design/" + prefix + "/" + id.section(':', 1) + "/"},
                {"author", info.value("author").toObject().value("name").toString()},
                {"license", info.value("license").toObject().value("title").toString()},
                {"licenseUrl", licenseUrl.scheme() == "https" ? licenseUrl.toString() : QString()},
                {"brand", prefix == "simple-icons"}});
            if (m_results.size() == 32)
                break;
        }
        emit resultsChanged();
        if (m_results.isEmpty())
            setError(tr("No matching icons. Try another name or choose an image."));
    });
}

bool CardIcons::selectImage(const QUrl &source)
{
    cancel();
    setError(QString());
    const QFileInfo file(source.toLocalFile());
    if (!source.isLocalFile() || !file.isFile() || file.size() > 64 * 1024 * 1024) {
        setError(tr("Please choose an image stored on the device."));
        return false;
    }
    QImageReader reader(file.absoluteFilePath());
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (!size.isValid() || qint64(size.width()) * size.height() > 64000000) {
        setError(tr("This image is too large or is not supported."));
        return false;
    }
    if (size.width() > 1024 || size.height() > 1024)
        reader.setScaledSize(size.scaled(1024, 1024, Qt::KeepAspectRatio));
    const QImage image = reader.read();
    if (image.isNull()) {
        setError(tr("Could not open the selected image."));
        return false;
    }
    setImage(image, {{"source", "image"}});
    return true;
}

void CardIcons::setImage(const QImage &image, const QVariantMap &credit)
{
    m_image = image;
    m_credit = credit;
    m_preview = "data:image/png;base64," + pngText(image);
    emit previewChanged();
}

QVariantMap CardIcons::preparedIcon(bool crop, qreal x, qreal y) const
{
    if (m_image.isNull() || !std::isfinite(x) || !std::isfinite(y))
        return {};
    QImage icon(256, 256, QImage::Format_RGB32);
    icon.fill(Qt::white);
    QPainter painter(&icon);
    if (crop) {
        const int side = qMin(m_image.width(), m_image.height());
        const QRect source(qRound(qBound(qreal(0), x, qreal(1)) * (m_image.width() - side)),
                           qRound(qBound(qreal(0), y, qreal(1)) * (m_image.height() - side)), side, side);
        painter.drawImage(icon.rect(), m_image.copy(source).scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    } else {
        const QImage fitted = m_image.scaled(224, 224, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawImage((256 - fitted.width()) / 2, (256 - fitted.height()) / 2, fitted);
    }
    painter.end();
    QVariantMap result = m_credit;
    result.remove("preview");
    result.insert("png", pngText(icon));
    return result;
}
