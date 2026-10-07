import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    readonly property bool showingCode: barcode.status === Image.Ready
    readonly property real smallIconSize: Math.min(Theme.iconSizeMedium, height * 0.16)

    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        y: cover.showingCode ? (codeSquare.y - height) / 2 : Theme.paddingLarge
        width: cover.showingCode ? cover.smallIconSize : Theme.iconSizeExtraLarge
        height: width
        source: "image://theme/harbour-passviewer"
        fillMode: Image.PreserveAspectFit
    }

    Rectangle {
        id: codeSquare

        anchors.centerIn: parent
        width: Math.max(0, Math.min(parent.width - 2 * Theme.paddingMedium,
                       parent.height - 2 * (cover.smallIconSize + 2 * Theme.paddingSmall)))
        height: width
        color: "white"
        visible: cover.showingCode

        Image {
            id: barcode

            anchors.fill: parent
            anchors.margins: Theme.paddingSmall
            source: appWindow.coverBarcodeSource
            fillMode: Image.PreserveAspectFit
            smooth: false
        }
    }

    Image {
        id: passIcon

        anchors.horizontalCenter: parent.horizontalCenter
        y: codeSquare.y + codeSquare.height
           + (cover.height - codeSquare.y - codeSquare.height - height) / 2
        width: cover.smallIconSize
        height: width
        source: appWindow.coverPassIconSource
        fillMode: Image.PreserveAspectFit
        visible: cover.showingCode

        Image {
            anchors.fill: parent
            source: "image://theme/harbour-passviewer"
            fillMode: Image.PreserveAspectFit
            visible: passIcon.status !== Image.Ready
        }
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        visible: !cover.showingCode
        text: qsTr("Pass Viewer")
        textFormat: Text.PlainText
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
    }
}
