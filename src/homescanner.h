#ifndef HOMESCANNER_H
#define HOMESCANNER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDateTime>
#include <QMimeDatabase>
#include <QLocale>
#include <QStandardPaths>

#include "zipfile.h"

class HomeScanner : public QObject
{
    Q_OBJECT
public:
    explicit HomeScanner(QObject *parent = 0);
    QVariantMap readPass(const QString &path);

signals:
    void passesFound(QVariantList list, QStringList paths, bool update);

public slots:
    void scanHome(bool update = false);
    void scanHome(QString path);

private:
    QVariantMap m_buildPass(QString zipname);
    void m_cleanJson(QString &data);
    bool m_localizePass(QJsonDocument &json, ZipFile &zip);
};

#endif // HOMESCANNER_H
