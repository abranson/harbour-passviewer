import QtQuick 2.6
import QtMultimedia 5.6
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import PassViewer 1.0

Page {
    id: page

    allowedOrientations: Orientation.Portrait

    property bool captured
    property var importedBarcode
    property string errorMessage
    property real maximumZoom: 1
    property point focusPosition
    readonly property bool showingSave: importedBarcode !== undefined
    readonly property bool scanning: status === PageStatus.Active && appWindow.applicationActive
                                     && !captured && !showingSave

    function chooseImage() {
        errorMessage = ""
        pageStack.push(imagePicker)
    }

    Component {
        id: passPicker

        FilePickerPage {
            title: qsTr("Open pass file")
            nameFilters: ["*.pkpass", "*.pkpasses"]
            popOnSelection: false
            onSelectedContentChanged: {
                if (selectedContent.toString().length)
                    appWindow.openPass(selectedContent.toString())
            }
        }
    }

    Component {
        id: imagePicker

        ImagePickerPage {
            id: picker

            property url pendingImage
            property bool working
            readonly property bool ready: status === PageStatus.Active && appWindow.applicationActive

            title: qsTr("Choose a barcode image")
            // Sailfish 5.2 supports holding the picker open until decoding ends.
            popOnSelection: false
            onReadyChanged: if (!ready) working = false
            onSelectedContentChanged: {
                if (selectedContent.toString().length > 0) {
                    pendingImage = selectedContent
                    selectedContent = ""
                }
            }

            Timer {
                interval: 100
                repeat: true
                running: picker.ready && picker.pendingImage.toString().length > 0 && !picker.working
                onTriggered: picker.working = imageScanner.scanFile(picker.pendingImage)
            }

            BarcodeScanner {
                id: imageScanner

                active: picker.ready
                onFound: {
                    // Change the underlying page before the single back transition.
                    page.importedBarcode = barcode
                    picker.pendingImage = ""
                    pageStack.pop(page)
                }
                onScanError: {
                    picker.pendingImage = ""
                    picker.working = false
                    notificator.bannerNotification(message, "")
                }
            }

            Rectangle {
                anchors.fill: parent
                color: "#88000000"
                visible: picker.pendingImage.toString().length > 0

                MouseArea { anchors.fill: parent }
                BusyIndicator {
                    anchors.centerIn: parent
                    size: BusyIndicatorSize.Large
                    running: parent.visible
                }
            }
        }
    }

    onScanningChanged: {
        if (!scanning) {
            focusTimer.stop()
            camera.unlock()
        }
    }

    function focusAt(x, y) {
        // contentRect includes the cropped edges. Map the tap back to the
        // sensor's coordinates, accounting for Sailfish's frame rotation.
        var rect = viewfinder.contentRect
        if (rect.width <= 0 || rect.height <= 0)
            return
        var u = Math.max(0, Math.min(1, (x - viewfinder.x - rect.x) / rect.width))
        var v = Math.max(0, Math.min(1, (y - viewfinder.y - rect.y) / rect.height))
        var point
        switch (camera.orientation) {
        case 90: point = Qt.point(1 - v, u); break
        case 180: point = Qt.point(1 - u, 1 - v); break
        case 270: point = Qt.point(v, 1 - u); break
        default: point = Qt.point(u, v); break
        }
        camera.unlock()
        focusPosition = Qt.point(x, y)
        focusTimer.restart()
        camera.focus.customFocusPoint = point
        camera.searchAndLock()
    }

    Timer {
        id: focusTimer

        interval: 5000
        onTriggered: camera.unlock()
    }

    Camera {
        id: camera

        position: Camera.BackFace
        cameraState: page.scanning ? Camera.ActiveState : Camera.UnloadedState
        focus.focusMode: focusTimer.running ? Camera.FocusAuto : Camera.FocusContinuous
        focus.focusPointMode: focusTimer.running ? Camera.FocusPointCustom : Camera.FocusPointAuto
        onCameraStatusChanged: {
            // Sailfish backends may only report this once the camera is active.
            if (cameraStatus === Camera.ActiveStatus)
                page.maximumZoom = Math.max(1, maximumDigitalZoom)
        }
        onError: page.errorMessage = errorString
    }

    BarcodeScanner {
        id: scanner

        active: page.scanning
        viewfinder: preview
        onFound: {
            page.captured = true
            pageStack.replace(Qt.resolvedUrl("SaveCard.qml"), { barcode: barcode })
        }
        onScanError: page.errorMessage = message
    }

    Loader {
        anchors.fill: parent
        active: page.showingSave
        sourceComponent: Component {
            SaveCardForm {
                barcode: page.importedBarcode
                topPadding: page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        visible: !page.showingSave
        contentHeight: body.y + body.height + Theme.paddingLarge

        Column {
            id: body

            y: page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader { title: qsTr("Import") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: qsTr("Keep the whole barcode or QR code inside the square. Tap to focus; pinch to zoom.")
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }

            Rectangle {
                id: preview

                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: width
                color: "black"
                clip: true

                VideoOutput {
                    id: viewfinder

                    readonly property real imageRatio: sourceRect.width > 0
                                                       ? sourceRect.height / sourceRect.width : 16 / 9

                    anchors.centerIn: parent
                    width: Math.max(parent.width, parent.height / imageRatio)
                    height: width * imageRatio
                    source: camera
                    // Sailfish's camera backend already orients the frames.
                    // Match the native camera's VideoOutput without extra rotation.
                    // Size the output itself to the image, then clip its parent;
                    // some Sailfish renderers still letterbox PreserveAspectCrop.
                    fillMode: VideoOutput.PreserveAspectFit
                }

                Rectangle {
                    x: page.focusPosition.x - width / 2
                    y: page.focusPosition.y - height / 2
                    width: Theme.itemSizeSmall
                    height: width
                    radius: width / 2
                    color: "transparent"
                    border.width: 2 * Theme.pixelRatio
                    border.color: camera.lockStatus === Camera.Locked ? Theme.highlightColor : "white"
                    visible: focusTimer.running
                }

                PinchArea {
                    id: zoomGesture

                    anchors.fill: parent
                    enabled: camera.cameraStatus === Camera.ActiveStatus
                    property real initialZoom: 1

                    onPinchStarted: {
                        initialZoom = camera.digitalZoom
                        focusTimer.stop()
                        camera.unlock()
                    }
                    // Amplify a short pinch while keeping zoom-in/out symmetric.
                    onPinchUpdated: camera.digitalZoom = Math.max(1, Math.min(page.maximumZoom,
                        initialZoom * Math.pow(pinch.scale, 3)))

                    MouseArea {
                        anchors.fill: parent
                        preventStealing: true
                        onClicked: page.focusAt(mouse.x, mouse.y)
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: zoomSlider.height + Theme.paddingSmall
                    color: "#88000000"
                    visible: page.maximumZoom > 1

                    Slider {
                        id: zoomSlider

                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        leftMargin: Theme.paddingLarge
                        rightMargin: Theme.paddingLarge
                        palette.colorScheme: Theme.LightOnDark
                        label: qsTr("Zoom")
                        minimumValue: 0
                        maximumValue: 1
                        stepSize: 0.01
                        enabled: camera.cameraStatus === Camera.ActiveStatus

                        onDownChanged: {
                            if (down) {
                                focusTimer.stop()
                                camera.unlock()
                            }
                        }
                        onValueChanged: {
                            if (down)
                                camera.digitalZoom = 1 + value * (page.maximumZoom - 1)
                        }
                    }

                    // Slider drags write to the camera; camera changes (including
                    // pinches) move the slider whenever it isn't being dragged.
                    Binding {
                        target: zoomSlider
                        property: "value"
                        when: !zoomSlider.down
                        value: page.maximumZoom > 1 ? (camera.digitalZoom - 1) / (page.maximumZoom - 1) : 0
                    }
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Choose image")
                onClicked: page.chooseImage()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Open pass file")
                onClicked: pageStack.push(passPicker)
            }

            Label {
                id: errorLabel

                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: page.errorMessage.length > 0
                text: page.errorMessage
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }
        }
        VerticalScrollDecorator {}
    }
}
