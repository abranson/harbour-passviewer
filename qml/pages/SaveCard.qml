import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property var barcode
    property string retryPage: "ScanCard.qml"

    SaveCardForm {
        barcode: page.barcode
        retryPage: page.retryPage
        topPadding: page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0
    }
}
