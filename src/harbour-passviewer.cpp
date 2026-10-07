#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QString>
#include <QDBusInterface>

#include "settingsstore.h"
#include "barcodeimageprovider.h"
#include "zipfileimageprovider.h"
#include "homewatcher.h"
#include "notificator.h"
#include "datetimeformat.h"
#include "currencyformat.h"
#include "passhandler.h"
#include "passdb.h"
#include "savedcards.h"
#include "barcodescanner.h"
#include "cardicons.h"
#include "barcodecodec.h"
#include "passimporter.h"
#include "passsharer.h"


int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    QDBusInterface other("ch.p2501.harbour-passviewer", "/ch/p2501/harbour_passviewer", "ch.p2501.harbour_passviewer");
    // check if another instance is running
    if (other.isValid()) {
        // yes, it's running, so signal it and exit
        QString origin;
        if (argc == 2)
            origin = argv[1];
        other.call("openPass", origin);
        return 0;
    }

    qmlRegisterType<CardIcons>("PassViewer", 1, 0, "CardIcons");
    qmlRegisterType<PassImporter>("PassViewer", 1, 0, "PassImporter");
    qmlRegisterType<BarcodeScanner>("PassViewer", 1, 0, "BarcodeScanner");
    SavedCards savedCards;
    view->rootContext()->setContextProperty("savedCards", &savedCards);

    PassSharer passSharer(&savedCards);
    view->rootContext()->setContextProperty("passSharer", &passSharer);

    BarcodeCodec barcodeCodec;
    view->rootContext()->setContextProperty("barcodeCodec", &barcodeCodec);

    SettingsStore settingsStore;
    view->rootContext()->setContextProperty("settingsStore", &settingsStore);

    view->engine()->addImageProvider("barcode", new BarcodeImageProvider());

    view->engine()->addImageProvider("zipimage", new ZipFileImageProvider());

    HomeWatcher homeWatcher;
    view->rootContext()->setContextProperty("homeWatcher", &homeWatcher);

    Notificator notificator;
    view->rootContext()->setContextProperty("notificator", &notificator);

    DateTimeFormat dateTimeFormat;
    view->rootContext()->setContextProperty("dateTimeFormat", &dateTimeFormat);

    CurrencyFormat currencyFormat;
    view->rootContext()->setContextProperty("currencyFormat", &currencyFormat);

    PassHandler passHandler;
    view->rootContext()->setContextProperty("passHandler", &passHandler);

    PassDB passDB;
    view->rootContext()->setContextProperty("passDB", &passDB);

    view->setSource(SailfishApp::pathTo("qml/harbour-passviewer.qml"));

    view->show();
    return app->exec();
}
