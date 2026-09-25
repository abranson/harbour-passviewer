import QtQuick 2.0
import Sailfish.Silica 1.0
import "../lib/utils.js" as Utils

Page {
    id: page

    //allowedOrientations: Orientation.All

    property string jsondata: ''
    property string path: ''
    property string barcodeType: "qr"
    property string barcodeEncoding: "iso-8859-1"
    property string barcodeContent: ""
    property string barcodeAltText: ""
    property string barcodeError

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: body.y + body.height + Theme.paddingLarge

        PullDownMenu {

            MenuItem {
                text: qsTr("Fullscreen Barcode")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("ShowCodeFullscreen.qml"), { barcodeContent: barcodeContent, barcodeEncoding: barcodeEncoding, barcodeType: barcodeType });
                }
            }
        }

        Column {
            id: body
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Theme.horizontalPageMargin
            anchors.rightMargin: Theme.horizontalPageMargin
            anchors.topMargin: Theme.paddingLarge + (page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0)
            spacing: Theme.paddingMedium

            Image {
                source: "image://zipimage" + path + "/logo.png"
                width: sourceSize.width * Theme.iconSizeSmall / sourceSize.height
                height: Theme.iconSizeSmall
            }

            Label {
                id: logoText
                text: ''
                textFormat: Text.PlainText
                color: Theme.highlightColor

                MouseArea {
                    anchors.fill: parent
                    onPressAndHold: Utils.copyText(logoText.text, Clipboard, notificator);
                }
            }

            Repeater {
                model: ListModel {
                    id: frontFields
                    ListElement { title: ''; value: '' }
                }

                Column {

                    Label {
                        text: title
                        textFormat: Text.PlainText
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.highlightColor
                        x: Theme.paddingLarge

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: Utils.copyText(title + ": " + value, Clipboard, notificator);
                        }
                    }

                    Label {
                        text: value
                        textFormat: Text.PlainText
                        color: Theme.highlightColor

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: Utils.copyText(title + ": " + value, Clipboard, notificator);
                        }
                    }
                }
            }

            Image {
                source: "image://zipimage" + path + "/footer.png"
                width: sourceSize.width != 0 ? body.width : 0
                height: sourceSize.width != 0 ? sourceSize.height * body.width / sourceSize.width : 0

            }

            Rectangle {
                visible: barcodeImage.width != 0
                width: barcodeImage.width + Theme.fontSizeMedium
                height: barcodeImage.height + Theme.fontSizeMedium
                color: 'white'

                Image {
                    id: barcodeImage
                    anchors.centerIn: parent
                    width: sourceSize.width !== 0 ? Utils.barcodeSize(sourceSize.width, sourceSize.height, body.width, Theme.fontSizeMedium)[0] : 0
                    height: sourceSize.height !== 0 ? Utils.barcodeSize(sourceSize.width, sourceSize.height, body.width, Theme.fontSizeMedium)[1] : 0
                    smooth: false
                    fillMode: Image.PreserveAspectFit
                    source: barcodeContent.length ? "image://barcode/" + barcodeType + "/" + barcodeEncoding + "/" + barcodeContent : "";
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: settingsStore.barcodeTap
                    onClicked: pageStack.push(Qt.resolvedUrl("ShowCodeFullscreen.qml"), { barcodeContent: barcodeContent, barcodeEncoding: barcodeEncoding, barcodeType: barcodeType })
                }
            }

            Label {
                text: barcodeAltText
                textFormat: Text.PlainText
                color: Theme.highlightColor
            }

            Label {
                width: parent.width
                visible: text.length > 0
                text: {
                    if (page.barcodeError.length)
                        return page.barcodeError
                    if (barcodeImage.status === Image.Error)
                        return qsTr("Could not render this barcode reliably.")
                    return ""
                }
                textFormat: Text.PlainText
                color: Theme.highlightColor
                wrapMode: Text.Wrap
            }

            Repeater {
                model: ListModel {
                    id: backFields
                    ListElement { title: ''; value: '' }
                }

                Column {

                    Label {
                        text: title
                        textFormat: Text.PlainText
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.highlightColor
                        x: Theme.paddingLarge

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: Utils.copyText(title + ": " + value, Clipboard, notificator);
                        }
                    }

                    Label {
                        text: value
                        textFormat: Text.PlainText
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.highlightColor
                        width: body.width
                        wrapMode: Text.Wrap

                        MouseArea {
                            anchors.fill: parent
                            onPressAndHold: Utils.copyText(title + ": " + value, Clipboard, notificator);
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }

    Component.onCompleted: {
        function setFields(pass, style, fieldType, target) {
            // set a field model from json
            var fields = [];
            if (fieldType in pass[style]) {
                for (var field = 0; field < pass[style][fieldType].length; field++) {
                    var data = pass[style][fieldType][field];
                    target.append({ title: String(data.label), value: String(data.value) });
                }
            }
        }

        // get general pass data
        var pass = JSON.parse(jsondata);
        if ('logoText' in pass)
            logoText.text = pass.logoText
        var styles = ["boardingPass", "coupon", "eventTicket", "storeCard", "generic"];
        var style = '';
        for (var key = 0; key < styles.length; key++) {
            if (styles[key] in pass) {
                style = styles[key];
                break;
            }
        }
        // complete undefined fields
        Utils.checkFields(pass, style);
        // set front field contents
        frontFields.clear();
        setFields(pass, style, 'headerFields', frontFields);
        setFields(pass, style, 'primaryFields', frontFields);
        setFields(pass, style, 'secondaryFields', frontFields);
        setFields(pass, style, 'auxiliaryFields', frontFields);
        var barcode = Utils.selectBarcode(pass, barcodeCodec);
        barcodeContent = "";
        barcodeType = barcode.type;
        barcodeEncoding = barcode.encoding;
        barcodeAltText = barcode.altText;
        barcodeError = barcode.error;
        barcodeContent = barcode.content;
        // set back field contents
        backFields.clear();
        setFields(pass, style, 'backFields', backFields);
    }
}
