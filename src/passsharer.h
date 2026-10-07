#ifndef PASSSHARER_H
#define PASSSHARER_H

#include <QObject>
#include <QString>

class SavedCards;

class PassSharer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
public:
    explicit PassSharer(SavedCards *cards, QObject *parent = nullptr);
    Q_INVOKABLE QString prepare(const QString &path);
    QString error() const { return m_error; }
signals:
    void errorChanged();
private:
    SavedCards *m_cards;
    QString m_error;
};

#endif
