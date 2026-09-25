#ifndef CARDICONS_H
#define CARDICONS_H

#include <QImage>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>

class QNetworkReply;

class CardIcons : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QString preview READ preview NOTIFY previewChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY previewChanged)
    Q_PROPERTY(QVariantMap credit READ credit NOTIFY previewChanged)
public:
    explicit CardIcons(QObject *parent = nullptr);
    ~CardIcons() override;
    bool busy() const { return !m_reply.isNull(); }
    QString error() const { return m_error; }
    QVariantList results() const { return m_results; }
    QString preview() const { return m_preview; }
    QSize imageSize() const { return m_image.size(); }
    QVariantMap credit() const { return m_credit; }
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void selectCatalog(const QString &id);
    Q_INVOKABLE bool selectImage(const QUrl &source);
    Q_INVOKABLE QVariantMap preparedIcon(bool crop, qreal x, qreal y) const;

signals:
    void busyChanged();
    void errorChanged();
    void resultsChanged();
    void previewChanged();

private:
    void cancel();
    void setError(const QString &error);
    void setImage(const QImage &image, const QVariantMap &credit);
    void request(const QUrl &url, const QVariantMap &credit = {});
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QVariantList m_results;
    QImage m_image;
    QVariantMap m_credit;
    QString m_preview;
    QString m_error;
};

#endif
