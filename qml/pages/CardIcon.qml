import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import PassViewer 1.0

Page {
    id: page

    allowedOrientations: Orientation.All
    property string cardId
    readonly property var card: savedCards.card(cardId)
    readonly property var credit: icons.preview.length ? icons.credit : (card.icon || {})
    property bool crop
    property real cropX: 0.5
    property real cropY: 0.5
    property string errorMessage

    function search() {
        searchField.focus = false
        errorMessage = ""
        icons.search(searchField.text)
    }

    CardIcons {
        id: icons

        onPreviewChanged: {
            page.crop = credit.source === "image"
            page.cropX = 0.5
            page.cropY = 0.5
            page.errorMessage = ""
            flickable.scrollToTop()
        }
    }

    Component {
        id: imagePicker

        ImagePickerPage {
            title: qsTr("Choose a card icon")
            onSelectedContentChanged: {
                if (selectedContent.toString().length)
                    icons.selectImage(selectedContent)
            }
        }
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: body.y + body.height + Theme.paddingLarge

        Column {
            id: body

            y: page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Card icon") }

            Rectangle {
                id: previewFrame

                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(parent.width / 2, Theme.itemSizeExtraLarge * 2)
                height: width
                color: "white"
                clip: true

                Image {
                    id: previewImage

                    readonly property bool candidate: icons.preview.length > 0
                    readonly property real imageWidth: candidate ? Math.max(1, icons.imageSize.width) : 256
                    readonly property real imageHeight: candidate ? Math.max(1, icons.imageSize.height) : 256
                    readonly property real imageScale: candidate && page.crop
                        ? Math.max(previewFrame.width / imageWidth, previewFrame.height / imageHeight)
                        : Math.min(previewFrame.width / imageWidth, previewFrame.height / imageHeight) * (candidate ? 0.875 : 1)
                    width: imageWidth * imageScale
                    height: imageHeight * imageScale
                    x: candidate && page.crop ? -(width - parent.width) * page.cropX : (parent.width - width) / 2
                    y: candidate && page.crop ? -(height - parent.height) * page.cropY : (parent.height - height) / 2
                    source: candidate ? icons.preview : (page.card.iconImage || "image://theme/harbour-passviewer")
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: page.crop && icons.preview.length > 0
                    preventStealing: true
                    property real previousX
                    property real previousY

                    onPressed: {
                        previousX = mouse.x
                        previousY = mouse.y
                    }
                    onPositionChanged: {
                        if (!pressed)
                            return
                        var dx = previewImage.width - width
                        var dy = previewImage.height - height
                        if (dx > 1)
                            page.cropX = Math.max(0, Math.min(1, page.cropX - (mouse.x - previousX) / dx))
                        if (dy > 1)
                            page.cropY = Math.max(0, Math.min(1, page.cropY - (mouse.y - previousY) / dy))
                        previousX = mouse.x
                        previousY = mouse.y
                    }
                }
            }

            TextSwitch {
                width: parent.width
                visible: icons.preview.length > 0
                text: qsTr("Crop to square")
                description: checked ? qsTr("Drag the image to adjust the crop.") : qsTr("Fit the whole image inside the icon.")
                checked: page.crop
                onClicked: page.crop = checked
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: icons.preview.length > 0
                enabled: !icons.busy
                text: qsTr("Use icon")
                onClicked: {
                    var icon = icons.preparedIcon(page.crop, page.cropX, page.cropY)
                    if (!icon.png) {
                        page.errorMessage = qsTr("Please choose a valid icon.")
                    } else if (savedCards.setIcon(page.cardId, icon)) {
                        pageStack.pop()
                    } else {
                        page.errorMessage = savedCards.error
                    }
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Choose image")
                onClicked: pageStack.push(imagePicker)
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !!page.card.iconImage
                text: qsTr("Use default icon")
                onClicked: {
                    if (savedCards.setIcon(page.cardId, {}))
                        pageStack.pop()
                    else
                        page.errorMessage = savedCards.error
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: page.credit.source === "Iconify"
                text: [page.credit.name, page.credit.author, page.credit.license].filter(function(value) { return !!value }).join(" · ")
                textFormat: Text.PlainText
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                wrapMode: Text.Wrap
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: page.credit.source === "Iconify"
                spacing: Theme.paddingSmall

                Button {
                    text: qsTr("Source")
                    preferredWidth: Theme.buttonWidthSmall
                    enabled: !!page.credit.sourceUrl
                    onClicked: Qt.openUrlExternally(page.credit.sourceUrl)
                }
                Button {
                    text: qsTr("License")
                    preferredWidth: Theme.buttonWidthSmall
                    enabled: !!page.credit.licenseUrl
                    onClicked: Qt.openUrlExternally(page.credit.licenseUrl)
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: !!page.credit.brand
                text: qsTr("Brand logos may have additional usage terms.")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                wrapMode: Text.Wrap
            }

            SectionHeader { text: qsTr("Online catalog · Iconify") }

            SearchField {
                id: searchField

                width: parent.width
                placeholderText: qsTr("Search icons or brands")
                EnterKey.iconSource: "image://theme/icon-m-search"
                EnterKey.onClicked: page.search()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Search")
                enabled: searchField.text.trim().length > 0 && !icons.busy
                onClicked: page.search()
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: icons.busy
                visible: running
                size: BusyIndicatorSize.Medium
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: text.length > 0
                text: page.errorMessage || icons.error
                textFormat: Text.PlainText
                color: Theme.highlightColor
                wrapMode: Text.Wrap
            }

            Flow {
                id: results

                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                spacing: Theme.paddingSmall

                Repeater {
                    model: icons.results

                    BackgroundItem {
                        width: (results.width - 2 * results.spacing) / 3
                        height: width + Theme.itemSizeSmall
                        enabled: !icons.busy
                        onClicked: icons.selectCatalog(modelData.id)

                        Rectangle {
                            x: Theme.paddingSmall
                            y: Theme.paddingSmall
                            width: parent.width - 2 * x
                            height: width
                            color: "white"

                            Image {
                                anchors.fill: parent
                                anchors.margins: Theme.paddingMedium
                                source: modelData.preview
                                sourceSize.width: 128
                                sourceSize.height: 128
                                asynchronous: true
                                fillMode: Image.PreserveAspectFit
                            }
                        }

                        Label {
                            y: parent.width
                            width: parent.width
                            text: modelData.name
                            textFormat: Text.PlainText
                            font.pixelSize: Theme.fontSizeExtraSmall
                            horizontalAlignment: Text.AlignHCenter
                            truncationMode: TruncationMode.Fade
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
