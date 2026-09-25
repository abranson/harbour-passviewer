import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All
    property string cardId
    property var card: savedCards.card(cardId)

    Connections {
        target: savedCards
        onChanged: page.card = savedCards.card(page.cardId)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: body.y + body.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Change icon")
                onClicked: pageStack.push(Qt.resolvedUrl("CardIcon.qml"), { cardId: page.cardId })
            }
            MenuItem {
                text: qsTr("Fullscreen Barcode")
                onClicked: page.showFullscreen()
            }
        }

        Column {
            id: body

            y: page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: page.card.name || qsTr("Saved card") }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.iconSizeLauncher
                height: width
                visible: source.toString().length > 0
                source: page.card.iconImage || ""
                fillMode: Image.PreserveAspectFit
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: page.card.format || ""
                textFormat: Text.PlainText
                color: Theme.highlightColor
            }

            Image {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                height: Math.min(width, sourceSize.height * width / Math.max(1, sourceSize.width))
                source: page.card.image || ""
                smooth: false
                fillMode: Image.PreserveAspectFit

                MouseArea {
                    anchors.fill: parent
                    onClicked: page.showFullscreen()
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: page.card.text || ""
                textFormat: Text.PlainText
                wrapMode: Text.WrapAnywhere
            }
        }
        VerticalScrollDecorator {}
    }

    function showFullscreen() {
        pageStack.push(Qt.resolvedUrl("ShowCodeFullscreen.qml"), { barcodeImageSource: card.image })
    }
}
