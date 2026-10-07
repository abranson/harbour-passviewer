import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import "pages"

ApplicationWindow
{
    id: appWindow

    // Older Silica versions do not expose display cutouts.
    readonly property real screenCutoutHeight: Screen.topCutout !== undefined ? Screen.topCutout.height : 0

    signal openPass(string origin)
    readonly property url coverBarcodeSource: pageStack.currentPage
            && pageStack.currentPage.coverBarcodeSource !== undefined
            ? pageStack.currentPage.coverBarcodeSource : ""
    readonly property url coverPassIconSource: pageStack.currentPage
            && pageStack.currentPage.coverIconSource !== undefined
            ? pageStack.currentPage.coverIconSource : ""
    initialPage: Component { FirstPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    ShareAction {
        id: shareAction

        mimeType: "application/vnd.apple.pkpass"
        title: qsTr("Share pass")
    }

    function sharePass(path) {
        var file = passSharer.prepare(path)
        if (file.length) {
            shareAction.resources = [file]
            shareAction.trigger()
        } else {
            notificator.bannerNotification(passSharer.error, "")
        }
    }

}


