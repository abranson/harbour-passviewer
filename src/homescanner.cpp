#include "homescanner.h"

HomeScanner::HomeScanner(QObject *parent) : QObject(parent)
{

}

void HomeScanner::scanHome(bool update) {
    // Only enumerate the app-owned library. External files require import.
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/passes";
    QDir().mkpath(directory);
    QVariantList passes;
    const auto files = QDir(directory).entryInfoList({"*.pkpass"}, QDir::Files | QDir::NoSymLinks);
    for (const QFileInfo &file : files) {
        const QVariantMap pass = m_buildPass(file.absoluteFilePath());
        if (!pass.isEmpty())
            passes.append(pass);
    }
    emit passesFound(passes, {directory}, update);
}

void HomeScanner::scanHome(QString) {
    scanHome(false);
}

QVariantMap HomeScanner::readPass(const QString &path) {
    return m_buildPass(path);
}

QVariantMap HomeScanner::m_buildPass(QString zipname) {
    // get json data from the file
    ZipFile zip(zipname);
    if (!zip.isValid())
        return QVariantMap();
    QString jsondata = zip.getTextFile("pass.json");
    if (jsondata == "")
        return QVariantMap();
    m_cleanJson(jsondata);
    QJsonDocument json(QJsonDocument::fromJson(jsondata.toUtf8()));
    if (json.isNull())
        return QVariantMap();
    // get the type id
    QString typeId = json.object().value("passTypeIdentifier").toString();
    if (typeId == "")
        return QVariantMap();
    // get the pass style
    QStringList styles({"boardingPass", "coupon", "eventTicket", "storeCard", "generic"});
    QString passStyle;
    for (auto style = styles.cbegin(); style != styles.cend(); ++style) {
        if (json.object().contains(*style)) {
            passStyle = *style;
            break;
        }
    }
    if (passStyle == "")
        return QVariantMap();
    // look for localization
    if (m_localizePass(json, zip))
        jsondata = QString(json.toJson());
    // create the pass name
    QString name;
    QJsonArray primaries(json.object().value(passStyle).toObject().value("primaryFields").toArray());
    QJsonArray secondaries(json.object().value(passStyle).toObject().value("secondaryFields").toArray());
    QJsonArray auxiliaries(json.object().value(passStyle).toObject().value("auxiliaryFields").toArray());
    if (passStyle == "boardingPass" && primaries.count() > 1)  // use parting point and destination
        name = primaries.at(0).toObject().value("value").toString() + " → " + primaries.at(1).toObject().value("value").toString();
    else if (primaries.count() > 0)
        name = primaries.at(0).toObject().value("label").toString() + " " + primaries.at(0).toObject().value("value").toString();
    else if (secondaries.count() > 0)
        name = secondaries.at(0).toObject().value("label").toString() + " " + secondaries.at(0).toObject().value("value").toString();
    else if (auxiliaries.count() > 0)
        name = auxiliaries.at(0).toObject().value("label").toString() + " " + auxiliaries.at(0).toObject().value("value").toString();
    // Passes in the private library have generated filenames.
    if (name.trimmed().isEmpty())
        name = json.object().value("description").toString();
    // use the file basename if everything else fails
    if (name == "" || name == " ")
        name = QFileInfo(zipname).baseName();
    const bool bundle = false;
    const bool updateable = json.object().contains("webServiceURL") && json.object().contains("serialNumber") && json.object().contains("authenticationToken");
    // construct and return the pass
    return QVariantMap({{"name", name}, {"path", zipname}, {"jsondata", jsondata}, {"typeId", typeId}, {"bundle", bundle}, {"updateable", updateable}, {"mtime", QFileInfo(zipname).lastModified()}});
}

void HomeScanner::m_cleanJson(QString &data) {
    // this finds and removes commas at the end of arrays and objects (empty elements)
    QRegularExpression findcomma("[\\]}]\\s*,\\s*[\\]}]");
    bool done = false;
    do {
        auto match = findcomma.match(data);
        if (match.hasMatch()) {
            int commapos = data.indexOf(',', match.capturedStart());
            if (commapos >= 0)
                data.remove(commapos, 1);
            else
                done = true; // for safety
        }
        else
            done = true;
    } while(!done);
    // this cleans any garbage at the end
    int objend = data.lastIndexOf('}') + 1;
    if (data.size() > objend)
        data.resize(objend);
}

bool HomeScanner::m_localizePass(QJsonDocument &json, ZipFile &zip) {
    // get local language
    QString lang(QLocale().bcp47Name());
    if (lang == "C")
        lang = "en";
    if (lang.contains("_"))
        lang = lang.left(lang.indexOf("_"));
    // get translation file
    QString transFile(zip.getTextFile(lang + ".lproj/pass.strings"));
    if (transFile == "")
        transFile = zip.getTextFile("en.lproj/pass.strings");
    if (transFile == "")
        return false;
    // parse translation file
    QMap<QString, QString> trans;
    bool instr = false;
    bool inkey = true;
    bool escaped = false;
    bool comment = false;
    QString key;
    QString value;
    for (int pos = 0; pos < transFile.size(); pos++) {
        QChar chr = transFile.at(pos);
        if (instr) {
            if (inkey)
                key.append(chr);
            else
                value.append(chr);
            if (escaped)
                escaped = false;
            else {
                if (chr == '\\')
                    escaped = true;
                if (chr == '"')
                    instr = false;
            }
        }
        else {
            if (comment) {
                if (transFile.mid(pos, 2) == "*/")
                    comment = false;
            }
            else {
                if (chr == '"') {
                    instr = true;
                    if (inkey)
                        key = QString("[\"");
                    else
                        value = QString("[\"");
                }
                if (chr == '=')
                    inkey = false;
                if (chr == ';') {
                    key.append("]");
                    key = QJsonDocument::fromJson(key.toUtf8()).array().at(0).toString();
                    value.append("]");
                    value = QJsonDocument::fromJson(value.toUtf8()).array().at(0).toString();
                    trans.insert(key, value);
                    inkey = true;
                }
                if (transFile.mid(pos, 2) == "/*")
                    comment = true;
            }
        }
    }
    // update json
    QStringList styles({"boardingPass", "coupon", "eventTicket", "storeCard", "generic"});
    QStringList types({"headerFields", "primaryFields", "secondaryFields", "auxiliaryFields", "backFields"});
    QStringList subTypes({"label", "value"});
    QJsonObject docObj(json.object());
    bool docChanged = false;
    for (auto style = styles.cbegin(); style != styles.cend(); ++style) {
        if (!docObj.contains(*style))
            continue;
        QJsonObject styleObj(docObj.value(*style).toObject());
        bool styleChanged = false;
        for (auto type = types.cbegin(); type != types.cend(); ++type) {
            if (!styleObj.contains(*type))
                continue;
            QJsonArray typeArray(styleObj.value(*type).toArray());
            bool typeChanged = false;
            for (int field = 0; field < typeArray.size(); field++) {
                QJsonObject fieldObj(typeArray.at(field).toObject());
                bool fieldChanged = false;
                for (auto subType = subTypes.cbegin(); subType != subTypes.cend(); ++subType) {
                    if (trans.contains(fieldObj.value(*subType).toString())) {
                        fieldObj.insert(*subType, trans.value(fieldObj.value(*subType).toString()));
                        fieldChanged = true;
                    }
                }
                if (fieldChanged) {
                    typeArray.replace(field, fieldObj);
                    typeChanged = true;
                }
            }
            if (typeChanged) {
                styleObj.insert(*type, typeArray);
                styleChanged = true;
            }
        }
        if (styleChanged) {
            docObj.insert(*style, styleObj);
            docChanged = true;
        }
    }
    if (docChanged)
        json.setObject(docObj);
    return docChanged;
}
