#ifndef SAVEDCARDS_H
#define SAVEDCARDS_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class SavedCards : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
public:
    explicit SavedCards(QObject *parent = nullptr);
    Q_INVOKABLE QVariantList passes() const;
    Q_INVOKABLE QVariantMap card(const QString &id) const;
    Q_INVOKABLE QString save(const QString &name, const QVariantMap &barcode);
    Q_INVOKABLE bool setIcon(const QString &id, const QVariantMap &icon);
    Q_INVOKABLE bool remove(const QString &id);
    QString error() const { return m_error; }

signals:
    void changed();
    void errorChanged();

private:
    QString filePath(const QString &id) const;
    void setError(const QString &error);
    QString m_directory;
    QString m_error;
};

#endif
