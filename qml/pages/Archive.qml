import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All
    property var passModel
    property Component itemDelegate

    SilicaListView {
        anchors.fill: parent
        model: page.passModel
        delegate: page.itemDelegate

        header: Column {
            width: parent.width

            Item {
                width: parent.width
                height: page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0
            }

            PageHeader { title: qsTr("Archive") }
        }

        ViewPlaceholder {
            enabled: page.passModel.count === 0
            text: qsTr("No archived passes")
        }

        VerticalScrollDecorator {}
    }
}
