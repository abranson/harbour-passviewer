import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    //allowedOrientations: Orientation.All

    property string name: ''
    property string jsondata: ''
    property string path: ''
    property bool updateable: false
    readonly property url coverIconSource: path.length ? "image://zipimage" + path + "/icon.png" : ""
    readonly property url coverBarcodeSource: pass.item ? pass.item.barcodeSource : ""

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: pass.y + pass.item.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Share")
                onClicked: appWindow.sharePass(page.path)
            }

            MenuItem {
                text: qsTr("Fullscreen Barcode")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ShowCodeFullscreen.qml"), { coverIconSource: page.coverIconSource, barcodeImageSource: pass.item.barcodeImageSource, barcodeContent: pass.item.barcodeContent, barcodeEncoding: pass.item.barcodeEncoding, barcodeType: pass.item.barcodeType });
                }
            }

            MenuItem {
                text: qsTr("Simple View")
                onClicked: {
                    var properties = { path: pass.item.path, jsondata: pass.item.jsondata };
                    pageStack.push(Qt.resolvedUrl("ShowSimple.qml"), properties);
                }
            }

            MenuItem {
                text: qsTr("Create Calendar Entry")
                enabled: pass.item.relevantDate !== ""
                onClicked: {
                    passHandler.createCalendarEntry(name, pass.item.relevantDate);
                }
            }

            MenuItem {
                text: qsTr("Update")
                enabled: updateable
                onClicked: {
                    passHandler.updatePass(page.path);
                }
            }
        }

        Loader {
            id: pass
            width: Math.min(parent.width - 2 * Theme.horizontalPageMargin, Theme.fontSizeMedium * 20)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Theme.paddingLarge + (page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0)
            source: Qt.resolvedUrl("../lib/Pass.qml")
            onLoaded: {
                item.path = path
                item.jsondata = jsondata
            }
        }

        VerticalScrollDecorator {}
    }
}
