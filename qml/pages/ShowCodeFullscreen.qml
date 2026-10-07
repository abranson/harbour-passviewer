import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    property url coverIconSource
    property url barcodeImageSource
    readonly property alias coverBarcodeSource: barcodeImage.source
    property string barcodeType: "qr"
    property string barcodeEncoding: "iso-8859-1"
    property string barcodeContent: ""

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: Theme.horizontalPageMargin
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.topMargin: Theme.paddingLarge + (page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0)
        anchors.bottomMargin: Theme.paddingLarge + (page.orientation === Orientation.PortraitInverted ? appWindow.screenCutoutHeight : 0)
        color: 'white'

        Image {
            id: barcodeImage

            anchors.fill: parent
            anchors.margins: Theme.paddingLarge
            smooth: false
            fillMode: Image.PreserveAspectFit
            source: page.barcodeImageSource.toString().length > 0 ? page.barcodeImageSource : barcodeContent.length ? "image://barcode/" + barcodeType + "/" + barcodeEncoding + "/" + barcodeContent : "";
        }

        Label {
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.paddingLarge
            visible: barcodeImage.status === Image.Error
            text: qsTr("Could not render this barcode reliably.")
            color: "black"
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
