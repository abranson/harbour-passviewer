import QtQuick 2.6
import Sailfish.Silica 1.0

SilicaFlickable {
    id: form

    property var barcode
    property string retryPage: "ScanCard.qml"
    property string saveError
    property real topPadding

    anchors.fill: parent
    contentHeight: body.y + body.height + Theme.paddingLarge

    Column {
        id: body

        y: form.topPadding
        width: parent.width
        spacing: Theme.paddingLarge

        PageHeader { title: qsTr("Save card") }

        TextField {
            id: nameField

            width: parent.width
            label: qsTr("Card name")
            placeholderText: qsTr("Store or card name")
            maximumLength: 200
            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            text: form.barcode.format
            textFormat: Text.PlainText
            color: Theme.highlightColor
        }

        Image {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            height: Math.min(width, sourceSize.height * width / Math.max(1, sourceSize.width))
            source: "data:image/png;base64," + form.barcode.png
            smooth: false
            fillMode: Image.PreserveAspectFit
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            text: form.barcode.text
            textFormat: Text.PlainText
            wrapMode: Text.WrapAnywhere
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Save")
            enabled: nameField.text.trim().length > 0
            onClicked: {
                var id = savedCards.save(nameField.text, form.barcode)
                if (id.length > 0)
                    pageStack.replace(Qt.resolvedUrl("ShowCard.qml"), { cardId: id })
                else
                    form.saveError = savedCards.error
            }
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Try another code")
            onClicked: pageStack.replace(Qt.resolvedUrl(form.retryPage))
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            visible: form.saveError.length > 0
            text: form.saveError
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            color: Theme.highlightColor
        }
    }
    VerticalScrollDecorator {}
}
