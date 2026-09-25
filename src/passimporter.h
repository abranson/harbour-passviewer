#ifndef PASSIMPORTER_H
#define PASSIMPORTER_H

#include <QObject>
#include <QFutureWatcher>
#include <QSharedPointer>
#include <QVariantList>

struct PendingPassImport;
struct PassImportResult {
    QString error;
    QStringList paths;
    bool changed = false;
};

class PassImporter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QVariantList passes READ passes NOTIFY changed)
    Q_PROPERTY(QString fileName READ fileName NOTIFY changed)
public:
    explicit PassImporter(QObject *parent = nullptr);
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    QVariantList passes() const { return m_passes; }
    QString fileName() const { return m_fileName; }
    Q_INVOKABLE void prepare(const QString &origin);
    Q_INVOKABLE void importPasses();

signals:
    void changed();
    void libraryChanged();
    void imported(QStringList paths);

private:
    QFutureWatcher<QSharedPointer<PendingPassImport>> m_prepare;
    QFutureWatcher<PassImportResult> m_import;
    QSharedPointer<PendingPassImport> m_pending;
    bool m_busy = false;
    QString m_error;
    QString m_fileName;
    QVariantList m_passes;
};

#endif
