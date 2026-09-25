import QtQuick 2.6
import Sailfish.Silica 1.0
import PassViewer 1.0

Page {
    id: page

    property string origin
    allowedOrientations: Orientation.All
    backNavigation: !importer.busy
    Component.onCompleted: importer.prepare(origin)

    PassImporter {
        id: importer

        onLibraryChanged: homeWatcher.scanHome()
        onImported: {
            notificator.bannerNotification(qsTr("Imported %1 pass(es)").arg(paths.length), "")
            pageStack.pop()
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: body.y + body.height + Theme.paddingLarge

        Column {
            id: body

            y: page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: {
                    if (importer.passes.length > 1)
                        return qsTr("Import %1 passes").arg(importer.passes.length)
                    return qsTr("Import pass")
                }
            }

            Repeater {
                model: importer.passes

                Column {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(body.width - 2 * Theme.horizontalPageMargin, Theme.fontSizeMedium * 20)
                    spacing: Theme.paddingMedium

                    Loader {
                        width: parent.width
                        source: Qt.resolvedUrl("../lib/Pass.qml")
                        onLoaded: {
                            // Previewing must not consume saved-pass update markers.
                            item.preview = true
                            item.path = modelData.path
                            item.jsondata = modelData.jsondata
                        }
                    }

                    Label {
                        width: parent.width
                        visible: modelData.replaces
                        text: qsTr("Updates saved pass")
                        wrapMode: Text.Wrap
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: importer.busy
                running: visible
                size: BusyIndicatorSize.Large
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: importer.error.length > 0
                text: importer.error
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Import")
                visible: importer.passes.length > 0
                enabled: !importer.busy
                onClicked: importer.importPasses()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Cancel")
                enabled: !importer.busy
                onClicked: pageStack.pop()
            }
        }

        VerticalScrollDecorator {}
    }
}
