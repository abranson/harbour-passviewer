#include "passimporter.h"
#include "homescanner.h"

#include <QCryptographicHash>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QUrl>
#include <QtConcurrent>

struct PendingPassImport {
    QTemporaryDir staging;
    QString error;
    QString fileName;
    QVariantList passes;
};

namespace {
const qint64 maximumBytes = 150 * 1024 * 1024;

QString libraryDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/passes";
}

QSharedPointer<PendingPassImport> prepareFile(const QString &origin)
{
    auto pending = QSharedPointer<PendingPassImport>::create();
    const QUrl url(origin);
    const QString path = url.isLocalFile() ? url.toLocalFile() : (url.scheme().isEmpty() ? origin : QString());
    QFile file(path);
    pending->fileName = QFileInfo(path).fileName();
    if (path.isEmpty() || !pending->staging.isValid() || !file.open(QIODevice::ReadOnly)
            || file.size() > maximumBytes || !QFileInfo(file).isFile()) {
        pending->error = PassImporter::tr("Could not open this pass file. Choose a local .pkpass or .pkpasses file up to 150 MB.");
        return pending;
    }
    // Snapshot the opened file before confirmation; never import a changed source later.
    const QByteArray source = file.read(maximumBytes + 1);
    if (source.isEmpty() || source.size() > maximumBytes || file.error() != QFile::NoError) {
        pending->error = PassImporter::tr("Could not read this pass file.");
        return pending;
    }
    const QString snapshot = QDir(pending->staging.path()).filePath("source");
    QFile stagedSource(snapshot);
    if (!stagedSource.open(QIODevice::WriteOnly) || stagedSource.write(source) != source.size()) {
        pending->error = PassImporter::tr("Could not prepare the import. Check available storage.");
        return pending;
    }
    stagedSource.close();
    ZipFile archive(snapshot);
    const bool bundle = QFileInfo(path).suffix().compare("pkpasses", Qt::CaseInsensitive) == 0;
    QStringList members;
    if (bundle) {
        for (const QString &entry : archive.getFileList()) {
            if (entry.endsWith(".pkpass", Qt::CaseInsensitive))
                members.append(entry);
        }
    } else {
        members.append(QString());
    }
    if (!archive.isValid() || members.isEmpty() || members.size() > 10) {
        pending->error = PassImporter::tr("This is not a supported pass file or bundle. A bundle can contain up to 10 passes.");
        return pending;
    }
    HomeScanner reader;
    qint64 total = 0;
    QMap<QString, QByteArray> identities;
    for (const QString &member : members) {
        const QByteArray bytes = bundle ? archive.getFile(member, maximumBytes - total) : source;
        total += bytes.size();
        // Member names are never used as filesystem paths.
        const QString stagedPath = QDir(pending->staging.path()).filePath(QString::number(pending->passes.size()) + ".pkpass");
        QFile staged(stagedPath);
        if (bytes.isEmpty() || total > maximumBytes || !staged.open(QIODevice::WriteOnly)
                || !staged.setPermissions(QFile::ReadOwner | QFile::WriteOwner)
                || staged.write(bytes) != bytes.size()) {
            pending->error = PassImporter::tr("Could not read every pass in this file. Nothing was imported.");
            break;
        }
        staged.close();
        QVariantMap pass = reader.readPass(stagedPath);
        if (pass.isEmpty()) {
            pending->error = PassImporter::tr("This file contains an invalid pass. Nothing was imported.");
            break;
        }
        const auto json = QJsonDocument::fromJson(pass.value("jsondata").toString().toUtf8()).object();
        const QString serial = json.value("serialNumber").toString();
        const QByteArray identity = serial.isEmpty() ? bytes : QJsonDocument(QJsonArray{
            json.value("passTypeIdentifier"), serial}).toJson(QJsonDocument::Compact);
        const QString key = QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
        const QByteArray digest = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
        if (identities.contains(key)) {
            if (identities.value(key) != digest) {
                pending->error = PassImporter::tr("This bundle contains conflicting versions of the same pass. Nothing was imported.");
                break;
            }
            continue;
        }
        identities.insert(key, digest);
        const QString target = libraryDirectory() + "/" + key + ".pkpass";
        QString name = pass.value("name").toString().trimmed();
        if (name == QFileInfo(stagedPath).baseName())
            name = json.value("description").toString();
        if (name.isEmpty())
            name = QFileInfo(bundle ? member : path).completeBaseName();
        pass.insert("name", name);
        pass.insert("target", target);
        pass.insert("replaces", QFile::exists(target));
        pending->passes.append(pass);
    }
    if (!pending->error.isEmpty())
        pending->passes.clear();
    return pending;
}

PassImportResult savePasses(const QSharedPointer<PendingPassImport> &pending)
{
    PassImportResult result;
    const QString directory = libraryDirectory();
    if (!QDir().mkpath(directory)) {
        result.error = PassImporter::tr("Could not create the pass library.");
        return result;
    }
    for (const QVariant &value : pending->passes) {
        const auto pass = value.toMap();
        const QString target = pass.value("target").toString();
        QFile source(pass.value("path").toString());
        QSaveFile destination(target);
        if (!source.open(QIODevice::ReadOnly) || !destination.open(QIODevice::WriteOnly)
                || !destination.setPermissions(QFile::ReadOwner | QFile::WriteOwner)) {
            result.error = PassImporter::tr("Import stopped after %1 pass(es). Check available storage and try again.").arg(result.paths.size());
            break;
        }
        bool written = true;
        while (!source.atEnd()) {
            const QByteArray chunk = source.read(65536);
            if (chunk.isEmpty() || destination.write(chunk) != chunk.size()) {
                written = false;
                break;
            }
        }
        if (!written || !destination.commit()) {
            result.error = PassImporter::tr("Import stopped after %1 pass(es). Check available storage and try again.").arg(result.paths.size());
            break;
        }
        result.paths.append(target);
        result.changed = true;
    }
    return result;
}
}

PassImporter::PassImporter(QObject *parent) : QObject(parent)
{
    connect(&m_prepare, &QFutureWatcher<QSharedPointer<PendingPassImport>>::finished, this, [this]() {
        m_pending = m_prepare.result();
        m_error = m_pending->error;
        m_passes = m_pending->passes;
        m_fileName = m_pending->fileName;
        m_busy = false;
        emit changed();
    });
    connect(&m_import, &QFutureWatcher<PassImportResult>::finished, this, [this]() {
        const auto result = m_import.result();
        m_error = result.error;
        m_busy = false;
        emit changed();
        if (result.changed)
            emit libraryChanged();
        if (m_error.isEmpty())
            emit imported(result.paths);
    });
}

void PassImporter::prepare(const QString &origin)
{
    if (m_busy)
        return;
    m_pending.clear();
    m_passes.clear();
    m_error.clear();
    m_busy = true;
    emit changed();
    m_prepare.setFuture(QtConcurrent::run(prepareFile, origin));
}

void PassImporter::importPasses()
{
    if (m_busy || !m_pending || m_passes.isEmpty())
        return;
    m_error.clear();
    m_busy = true;
    emit changed();
    m_import.setFuture(QtConcurrent::run(savePasses, m_pending));
}
