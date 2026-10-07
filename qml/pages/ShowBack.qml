import QtQuick 2.0
import Sailfish.Silica 1.0
import "../lib/utils.js" as Utils

Page {
    id: page

    //allowedOrientations: Orientation.All

    property string jsondata: ''
    property string path: ''
    readonly property url coverIconSource: path.length ? "image://zipimage" + path + "/icon.png" : ""
    readonly property url coverBarcodeSource: {
        if (!jsondata.length)
            return ""
        var data = JSON.parse(jsondata)
        var image = Utils.sharedBarcodeImage(data, path)
        if (image.length)
            return image
        var barcode = Utils.selectBarcode(data, barcodeCodec)
        return barcode.content.length
                ? "image://barcode/" + barcode.type + "/" + barcode.encoding + "/" + barcode.content : ""
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: pass.y + pass.item.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Share")
                onClicked: appWindow.sharePass(page.path)
            }

            MenuItem {
                text: qsTr("Simple View")
                onClicked: {
                    var properties = { path: path, jsondata: jsondata };
                    pageStack.push(Qt.resolvedUrl("ShowSimple.qml"), properties);
                }
            }
        }

        Loader {
            id: pass
            width: Math.min(parent.width - 2 * Theme.horizontalPageMargin, Theme.fontSizeMedium * 20)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Theme.paddingLarge + (page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0)
            source: Qt.resolvedUrl("../lib/Back.qml")
            onLoaded: {
                item.jsondata = jsondata
            }
        }

        VerticalScrollDecorator {}
    }
}
