#include <QtTest>
#include <QFile>
#include <QPainter>
#include <QTemporaryDir>
#include <limits>
#include "../src/cardicons.h"

class CardIconsTest : public QObject
{
    Q_OBJECT
private slots:
    void cropAndFit();
    void invalidImages();
};

static QImage rendered(const CardIcons &icons, bool crop, qreal x, qreal y)
{
    return QImage::fromData(QByteArray::fromBase64(icons.preparedIcon(crop, x, y).value("png").toByteArray()), "PNG");
}

void CardIconsTest::cropAndFit()
{
    QTemporaryDir directory;
    QImage wide(800, 400, QImage::Format_RGB32);
    wide.fill(Qt::red);
    QPainter painter(&wide);
    painter.fillRect(400, 0, 400, 400, Qt::blue);
    painter.end();
    const QString path = directory.filePath("wide.png");
    QVERIFY(wide.save(path));
    CardIcons icons;
    QSignalSpy preview(&icons, &CardIcons::previewChanged);
    QVERIFY(icons.selectImage(QUrl::fromLocalFile(path)));
    QCOMPARE(preview.count(), 1);
    QCOMPARE(icons.imageSize(), wide.size());
    QVERIFY(QFile::remove(path)); // The selected image remains available without its source file.
    const auto left = rendered(icons, true, 0, 0.5);
    const auto right = rendered(icons, true, 1, 0.5);
    QCOMPARE(left.size(), QSize(256, 256));
    QCOMPARE(left.pixel(128, 128), QColor(Qt::red).rgb());
    QCOMPARE(right.pixel(128, 128), QColor(Qt::blue).rgb());
    QCOMPARE(rendered(icons, true, -10, 10), left);
    const auto fitted = rendered(icons, false, 0.5, 0.5);
    QCOMPARE(fitted.pixel(128, 20), QColor(Qt::white).rgb());
    QCOMPARE(fitted.pixel(30, 128), QColor(Qt::red).rgb());
    QCOMPARE(fitted.pixel(220, 128), QColor(Qt::blue).rgb());
    QCOMPARE(icons.preparedIcon(true, 0, 0).value("source").toString(), QString("image"));
    QVERIFY(icons.preparedIcon(true, std::numeric_limits<qreal>::quiet_NaN(), 0).isEmpty());

    QImage tall(400, 800, QImage::Format_RGB32);
    tall.fill(Qt::red);
    QPainter tallPainter(&tall);
    tallPainter.fillRect(0, 400, 400, 400, Qt::blue);
    tallPainter.end();
    const QString tallPath = directory.filePath("tall.png");
    QVERIFY(tall.save(tallPath));
    QVERIFY(icons.selectImage(QUrl::fromLocalFile(tallPath)));
    QCOMPARE(rendered(icons, true, 0.5, 0).pixel(128, 128), QColor(Qt::red).rgb());
    QCOMPARE(rendered(icons, true, 0.5, 1).pixel(128, 128), QColor(Qt::blue).rgb());
}

void CardIconsTest::invalidImages()
{
    CardIcons icons;
    QVERIFY(icons.preparedIcon(false, 0.5, 0.5).isEmpty());
    QVERIFY(!icons.selectImage(QUrl("https://example.com/icon.png")));
    QVERIFY(!icons.selectImage(QUrl::fromLocalFile("/nonexistent-icon.png")));
    QTemporaryDir directory;
    const QString path = directory.filePath("broken.png");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("not an image");
    file.close();
    QVERIFY(!icons.selectImage(QUrl::fromLocalFile(path)));
    QVERIFY(!icons.error().isEmpty());
    QVERIFY(icons.preview().isEmpty());
    icons.search("  ");
    QVERIFY(!icons.busy());
    QVERIFY(icons.error().isEmpty());
    icons.selectCatalog("mdi:unknown");
    QVERIFY(!icons.busy());
}

QTEST_GUILESS_MAIN(CardIconsTest)
#include "tst_cardicons.moc"
