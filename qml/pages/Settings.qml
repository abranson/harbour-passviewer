import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: body.y + body.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Copyright")
                onClicked: pageStack.push(Qt.resolvedUrl("Copyright.qml"))
            }
        }

        Column {
            id: body

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: Theme.horizontalPageMargin
            anchors.rightMargin: Theme.horizontalPageMargin
            anchors.topMargin: Theme.paddingLarge + (page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0)
            spacing: Theme.paddingMedium

            Label {
                anchors.right: parent.right
                text: qsTr("Time")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
            }

            TextSwitch {
                id: time

                text: qsTr("Highlight passes close to event time")
                checked: settingsStore.checkTime
                onCheckedChanged: settingsStore.checkTime = checked
            }

            Slider {
                width: parent.width
                enabled: time.checked
                visible: time.checked
                stepSize: 1
                minimumValue: 0
                maximumValue: 8
                value: settingsStore.hoursBefore
                valueText: value + "h"
                label: qsTr("Time before event")
                onDownChanged: if (!down) settingsStore.hoursBefore = value
            }

            Slider {
                width: parent.width
                enabled: time.checked
                visible: time.checked
                stepSize: 1
                minimumValue: 0
                maximumValue: 8
                value: settingsStore.hoursAfter
                valueText: value + "h"
                label: qsTr("Time after event")
                onDownChanged: if (!down) settingsStore.hoursAfter = value
            }

            Slider {
                id: archiveDelay

                readonly property var hours: [0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 24, 48, 72, 168, 336, 720, -1]

                width: parent.width
                minimumValue: 0
                maximumValue: hours.length - 1
                stepSize: 1
                value: Math.max(0, hours.indexOf(settingsStore.archiveAfterHours))
                valueText: {
                    var delay = hours[Math.round(value)]
                    if (delay < 0)
                        return qsTr("Never")
                    if (delay === 0)
                        return qsTr("Immediately")
                    if (delay < 24)
                        return qsTr("%1 h").arg(delay)
                    if (delay === 24)
                        return qsTr("1 day")
                    return qsTr("%1 days").arg(delay / 24)
                }
                label: qsTr("Auto-archive after")
                onDownChanged: if (!down) settingsStore.archiveAfterHours = hours[Math.round(value)]
            }

            Label {
                width: parent.width
                text: qsTr("After the pass expiry or event time. Restored items stay in the main list.")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                wrapMode: Text.Wrap
            }

            Label {
                anchors.right: parent.right
                text: qsTr("Distance")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
            }

            TextSwitch {
                id: distance

                text: qsTr("Highlight passes close to destination")
                checked: settingsStore.checkDistance
                onCheckedChanged: settingsStore.checkDistance = checked
            }

            TextSwitch {
                automaticCheck: distance.checked
                visible: distance.checked
                text: qsTr("Prefer non-satellite position fixing")
                description: qsTr("Saves battery. <b>Requires</b> &quot;Faster position fix&quot; switched on in the system settings.")
                checked: settingsStore.useHere
                onCheckedChanged: settingsStore.useHere = checked
            }

            Slider {
                width: parent.width
                enabled: distance.checked
                visible: distance.checked
                stepSize: 50
                minimumValue: 100
                maximumValue: 2000
                value: settingsStore.maxDistance
                valueText: value + "m"
                label: qsTr("Distance to destination")
                onDownChanged: if (!down) settingsStore.maxDistance = value
            }

            TextSwitch {
                automaticCheck: distance.checked
                visible: distance.checked
                text: qsTr("Allow passes to override distance")
                checked: settingsStore.overrideDistance
                onCheckedChanged: settingsStore.overrideDistance = checked
            }

            Label {
                anchors.right: parent.right
                text: qsTr("Barcode")
                font.family: Theme.fontFamilyHeading
                color: Theme.highlightColor
            }

            TextSwitch {
                text: qsTr("Show barcode fullscreen when tapped")
                checked: settingsStore.barcodeTap
                onCheckedChanged: settingsStore.barcodeTap = checked
            }

        }

        VerticalScrollDecorator {}
    }
}
