import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    //allowedOrientations: Orientation.All

    property string jsondata: ''
    property string path: ''

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: pass.y + pass.item.height + Theme.paddingLarge

        PullDownMenu {

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
