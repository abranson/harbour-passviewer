import QtQuick 2.6
import Sailfish.Silica 1.0
import QtPositioning 5.2
import Nemo.DBus 2.0
import "../lib/timeline.js" as Timeline


Page {
    id: page

    allowedOrientations: Orientation.All

    property string uid: "firstPage"
    property bool hasImportedPasses
    property bool wide: hasImportedPasses && (Screen.sizeCategory == Screen.Large || Screen.sizeCategory == Screen.ExtraLarge) && (appWindow.orientation == Orientation.Landscape || appWindow.orientation == Orientation.LandscapeInverted)
    property bool displayOn: true
    property string pendingOpen

    ListModel {
        id: passList
    }

    ListModel {
        id: visiblePasses
    }

    ListModel {
        id: archivedPasses
    }

    Component {
        id: passDelegate

        ListItem {
            id: entry

            contentHeight: passIcon.height + Theme.paddingSmall * 2

            Image {
                id: passIcon
                width: Theme.iconSizeLauncher
                height: width
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: Theme.horizontalPageMargin
                source: path.indexOf("card:") === 0 ? (iconImage || "image://theme/harbour-passviewer") : "image://zipimage" + path + "/icon.png"
                fillMode: Image.PreserveAspectFit
            }

            Label {
                text: name
                textFormat: Text.PlainText
                font.bold: current || (passDisplay.status === Loader.Ready && passDisplay.item.path === path)
                width: parent.width - passIcon.width - Theme.horizontalPageMargin * 2 - Theme.paddingMedium
                truncationMode: TruncationMode.Fade
                color: entry.highlighted ? Theme.highlightColor : archived || past ? Theme.secondaryColor : Theme.primaryColor
                anchors.left: passIcon.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: Theme.paddingMedium
            }

            Label {
                text: relevantDate
                textFormat: Text.PlainText
                horizontalAlignment: Text.AlignRight
                font.pixelSize: Theme.fontSizeTiny
                width: parent.width - passIcon.width - Theme.horizontalPageMargin * 2 - Theme.paddingMedium
                truncationMode: TruncationMode.Fade
                color: entry.highlighted ? Theme.highlightColor : archived || past ? Theme.secondaryColor : Theme.primaryColor
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.bottomMargin: Theme.paddingSmall
            }

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Share")
                    onClicked: appWindow.sharePass(path)
                }


                MenuItem {
                    text: qsTr("Show")
                    onClicked: {
                        if (path.indexOf("card:") === 0) {
                            pageStack.push(Qt.resolvedUrl("ShowCard.qml"), { cardId: path.substring(5) });
                            return;
                        }
                        var properties = { name: name, path: path, jsondata: jsondata, updateable: updateable };
                        pageStack.push(Qt.resolvedUrl("ShowPass.qml"), properties);
                        pageStack.pushAttached(Qt.resolvedUrl("ShowBack.qml"), properties);
                    }
                }

                MenuItem {
                    text: qsTr("Change icon")
                    visible: path.indexOf("card:") === 0
                    onClicked: pageStack.push(Qt.resolvedUrl("CardIcon.qml"), { cardId: path.substring(5) })
                }

                MenuItem {
                    text: qsTr("Update")
                    visible: updateable
                    onClicked: {
                        passHandler.updatePass(path);
                    }
                }

                MenuItem {
                    text: archived ? qsTr("Restore") : qsTr("Move to archive")
                    onClicked: {
                        if (!settingsStore.setArchiveState(archiveKey, archived ? 2 : 1))
                            notificator.bannerNotification(qsTr("Could not change archive status"), "");
                    }
                }

                MenuItem {
                    text: qsTr("Delete")
                    visible: !bundle
                    onClicked: {
                        var delPath = path;
                        deleteRemorse.execute(entry, qsTr("Deleting"), function(){
                            if (delPath.indexOf("card:") === 0) {
                                if (!savedCards.remove(delPath.substring(5))) {
                                    notificator.bannerNotification(savedCards.error, "");
                                    return;
                                }
                            } else {
                                passHandler.removePass(delPath);
                            }
                            removePass(delPath);
                            refreshVisiblePasses();
                        });
                    }
                }
            }

            onClicked: openPass(path, false, true)

            ListView.onAdd: AddAnimation {
                target: entry
            }

            ListView.onRemove: RemoveAnimation {
                target: entry
            }

            RemorseItem {
                id: deleteRemorse
            }
        }
    }

    Row {
        anchors.fill: parent

        SilicaListView {
            id: passView

            width: page.wide ? parent.width - passColumn.width : parent.width
            height: parent.height

            header: Column {
                width: parent.width

                Item {
                    width: parent.width
                    height: page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0
                }

                PageHeader {
                    title: qsTr("Pass Viewer")
                }
            }

            PullDownMenu {

                MenuItem {
                    text: qsTr("Import")
                    onClicked: pageStack.push(Qt.resolvedUrl("ScanCard.qml"))
                }

                MenuItem {
                    text: qsTr("Archive (%1)").arg(archivedPasses.count)
                    onClicked: pageStack.push(Qt.resolvedUrl("Archive.qml"), {
                        passModel: archivedPasses, itemDelegate: passDelegate
                    })
                }

                MenuItem {
                    text: qsTr("Settings")
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("Settings.qml"));
                    }
                }
            }

            model: visiblePasses

            delegate: passDelegate

            VerticalScrollDecorator {}
        }

        SilicaFlickable {
            visible: page.wide && (!busy.running) && passList.count > 0
            width: Math.min(parent.width / 2, Theme.fontSizeMedium * 20)
            height: parent.height
            contentHeight: passColumn.height + 2 * Theme.paddingLarge

            Column {
                id: passColumn
                width: parent.width
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: Theme.paddingLarge
                spacing: Theme.paddingLarge

                Loader {
                    id: passDisplay

                    onLoaded: page.selectVisiblePass()
                    active: page.wide
                    enabled: parent.enabled
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: Qt.resolvedUrl("../lib/Pass.qml")
                }

                Button {
                    text: qsTr("Create Calendar Entry")
                    visible: passDisplay.status === Loader.Ready && passDisplay.item.relevantDate !== ""
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: {
                        passHandler.createCalendarEntry(passList.get(getPass(passDisplay.item.path)).name, passDisplay.item.relevantDate);
                    }
                }

                Button {
                    text: qsTr("Update")
                    visible: passDisplay.status === Loader.Ready && getPass(passDisplay.item.path) !== null
                             && passList.get(getPass(passDisplay.item.path)).updateable
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: {
                        passHandler.updatePass(passDisplay.item.path);
                    }
                }

                Loader {
                    id: backDisplay

                    onLoaded: page.selectVisiblePass()
                    active: page.wide
                    enabled: parent.enabled
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: Qt.resolvedUrl("../lib/Back.qml")
                }
            }

            VerticalScrollDecorator {}
        }
    }

    BusyIndicator {
        id: busy
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: true
    }

    Label {
        anchors.centerIn: parent
        text: qsTr("No passes or cards")
        color: Theme.highlightColor
        visible: visiblePasses.count == 0 && !busy.running
    }

    Timer {
        id: checkTimer
        interval: 60000
        repeat: true
        onTriggered: checkPassList()
    }

    PositionSource {
        id: locator
        property bool precise: false
        active: settingsStore.checkDistance && page.displayOn
        updateInterval: 60000
        preferredPositioningMethods: (precise || !settingsStore.useHere) ? PositionSource.AllPositioningMethods : PositionSource.NonSatellitePositioningMethods
        onPositionChanged: checkPassList()
    }

    Component.onCompleted: {
        // Load the app-owned library, without searching personal folders.
        passList.clear();
        homeWatcher.scanHome();
    }

    Connections {
        target: homeWatcher
        onPassesFound: {
            var cards = savedCards.passes();
            for (var card = 0; card < cards.length; card++)
                list.push(cards[card]);
            // check for vanished passes...
            var removePasses = [];
            for (var oldpass = 0; oldpass < passList.count; oldpass++) {
                var found = false;
                for (var newpass = 0; newpass < list.length; newpass++) {
                    if (passList.get(oldpass).path === list[newpass].path) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    removePasses.push(passList.get(oldpass).path);
            }
            // ...and remove them
            for (var toRemove = 0; toRemove < removePasses.length; toRemove++)
                removePass(removePasses[toRemove]);
            // calculate the points sort the list, and update GPS precision
            var close = false;
            for (var pass = 0; pass < list.length; pass++) {
                if (calcPointsAndTime(list[pass]))
                    close = true;
            }
            list.sort(comparePasses);
            if (locator.precise !== close)
                locator.precise = close;
            // update the pass list
            updatePasses(list);
            // on the first run: stop the busy animation, start the check timer and show the pass called in the CLI (if given)
            if (busy.running) {
                busy.running = false;
                checkTimer.start();
                var origin = page.pendingOpen || (Qt.application.arguments.length === 2 ? Qt.application.arguments[1] : "");
                page.pendingOpen = "";
                if (origin.length)
                    openPass(origin);
            }
            // report a successful update and redraw the pass, if it's shown
            if (update) {
                notificator.bannerNotification(qsTr("pass update successful"), "");
                if (pageStack.depth > 1)
                    openPass(pageStack.nextPage().path);
                if (page.wide && passDisplay.item.path !== '') {
                    try {
                        var showPass = passList.get(getPass(passDisplay.item.path));
                        passDisplay.item.path = showPass.path;
                        passDisplay.item.jsondata = showPass.jsondata;
                        backDisplay.item.jsondata = showPass.jsondata;
                    }
                    catch(e) {}
                }
            }
            selectVisiblePass();
        }
    }

    Connections {
        target: savedCards
        onChanged: homeWatcher.scanHome()
    }

    Connections {
        target: settingsStore
        onArchiveAfterHoursChanged: checkPassList()
        onArchiveStateChanged: checkPassList()
        onCheckTimeChanged: checkPassList()
        onHoursBeforeChanged: checkPassList()
        onHoursAfterChanged: checkPassList()
        onCheckDistanceChanged: checkPassList()
        onMaxDistanceChanged: checkPassList()
        onOverrideDistanceChanged: checkPassList()
    }

    Connections {
        target: passHandler
        onUpdateFinished: {
            switch (state) {
            case "not updateable":
                notificator.bannerNotification(qsTr("pass not updateable"), "");
                break;
            case "no new version":
                notificator.bannerNotification(qsTr("no new version for pass"), "");
                break;
            case "update failed":
                notificator.bannerNotification(qsTr("pass update failed"), "");
                break;
            case "ok":
                homeWatcher.scanHome(true);
            }
        }
        onCalendarEntryFinished: {
            if (state === "format")
                notificator.bannerNotification(qsTr("Format Error"), qsTr("Couldn't recognize date/time format"));
            if (state === "xdg-open")
                notificator.bannerNotification(qsTr("Unsupported"), qsTr("Please update your system or install calendar"));
        }
    }

    Connections {
        target: appWindow
        onOpenPass: {
            openPass(origin);
        }
        onOrientationChanged: selectVisiblePass()
    }

    DBusAdaptor {
        id: dbus
        service: "ch.p2501.harbour-passviewer"
        iface: "ch.p2501.harbour_passviewer"
        path: "/ch/p2501/harbour_passviewer"
        xml: '<interface name="ch.p2501.harbour_passviewer">' +
             '  <method name="openPass">' +
             '    <arg name="origin" type="s" direction="in"/>' +
             '  </method>' +
             '</interface>'
        function openPass(origin) {
            page.openPass(origin);
        }
    }

    DBusInterface {
        id: dbus_mce
        bus: DBus.SystemBus
        iface: 'com.nokia.mce.signal'
        path: '/com/nokia/mce/signal'
        service: 'com.nokia.mce'
        signalsEnabled: true
        function display_status_ind(status) {
            if (status === "off")
                page.displayOn = false;
            else
                page.displayOn = true;
        }
    }

    function openPass(origin, immediate, keepListPage) {
        if (typeof immediate === 'undefined')
            immediate = true;
        // bring the app to the foreground
        appWindow.activate();
        if (!origin || origin.length === 0)
            return;
        if (busy.running) {
            page.pendingOpen = origin;
            return;
        }
        if (origin.indexOf("card:") === 0) {
            if (!keepListPage)
                pageStack.pop(page, PageStackAction.Immediate);
            pageStack.push(Qt.resolvedUrl("ShowCard.qml"), { cardId: origin.substring(5) });
            return;
        }
        // get the canonical path
        origin = passHandler.getCanonicalPath(origin);
        // look for a matching pass
        var pass = getPass(origin);
        if (pass !== null) {
            // found one: let's show it
            if (pageStack.currentPage === page && passDisplay.status === Loader.Ready && backDisplay.status === Loader.Ready) {
                // on wide screen
                passDisplay.item.path = passList.get(pass).path;
                passDisplay.item.jsondata = passList.get(pass).jsondata;
                backDisplay.item.jsondata = passList.get(pass).jsondata;
            }
            else {
                // on small screen
                var properties = { name: passList.get(pass).name, path: passList.get(pass).path, jsondata: passList.get(pass).jsondata, updateable: passList.get(pass).updateable };
                if (!keepListPage)
                    pageStack.pop(page, PageStackAction.Immediate);
                if (immediate)
                    pageStack.push(Qt.resolvedUrl("ShowPass.qml"), properties, PageStackAction.Immediate);
                else
                    pageStack.push(Qt.resolvedUrl("ShowPass.qml"), properties);
                pageStack.pushAttached(Qt.resolvedUrl("ShowBack.qml"), properties);
            }
        }
        else {
            pageStack.pop(page, PageStackAction.Immediate);
            pageStack.push(Qt.resolvedUrl("ImportPass.qml"), { origin: origin });
        }
    }

    function updatePasses(newpasses) {
        // inserts, updates or moves the passes in the model
        for (var pass = 0; pass < newpasses.length; pass++) {
            var oldpoints = -1;
            if (pass < passList.count && passList.get(pass).path === newpasses[pass].path) {
                // update
                oldpoints = passList.get(pass).points;
                passList.set(pass, newpasses[pass]);
            }
            else {
                // check if it's further down
                var moved = false;
                for(var oldpass = pass + 1; oldpass < passList.count; oldpass++) {
                    if (passList.get(oldpass).path === newpasses[pass].path) {
                        // move and update
                        oldpoints = passList.get(oldpass).points;
                        passList.move(oldpass, pass, 1);
                        passList.set(pass, newpasses[pass]);
                        moved = true;
                        break;
                    }
                }
                if (!moved)
                    passList.insert(pass, newpasses[pass]);  // new pass
            }
            // update pass notifications
            if (oldpoints === -1 && newpasses[pass].points !== -1)
                notificator.addNotification(newpasses[pass].path, newpasses[pass].name, '');
            if (oldpoints !== -1 && newpasses[pass].points === -1)
                notificator.removeNotification(newpasses[pass].path);
        }
        refreshVisiblePasses();
    }

    function refreshVisiblePasses() {
        var rows = [];
        var archiveRows = [];
        for (var index = 0; index < passList.count; index++) {
            var pass = passList.get(index);
            var row = {};
            for (var key in pass)
                row[key] = pass[key];
            if (pass.archived)
                archiveRows.push(row);
            else
                rows.push(row);
        }
        page.hasImportedPasses = rows.some(function(row) { return row.path.indexOf("card:") !== 0; });
        updateVisibleModel(visiblePasses, rows);
        updateVisibleModel(archivedPasses, archiveRows);
        selectVisiblePass();
    }

    function updateVisibleModel(model, rows) {
        // Update in place so minute-by-minute checks don't reset scrolling.
        for (var rowIndex = 0; rowIndex < rows.length; rowIndex++) {
            var found = -1;
            for (var old = rowIndex; old < model.count; old++) {
                if (model.get(old).path === rows[rowIndex].path) {
                    found = old;
                    break;
                }
            }
            if (found < 0) {
                model.insert(rowIndex, rows[rowIndex]);
            } else {
                if (found !== rowIndex)
                    model.move(found, rowIndex, 1);
                model.set(rowIndex, rows[rowIndex]);
            }
        }
        if (model.count > rows.length)
            model.remove(rows.length, model.count - rows.length);
    }

    function selectVisiblePass() {
        if (!page.wide || passDisplay.status !== Loader.Ready || backDisplay.status !== Loader.Ready)
            return;
        var first = null;
        for (var index = 0; index < visiblePasses.count; index++) {
            var pass = visiblePasses.get(index);
            if (pass.path.indexOf("card:") !== 0) {
                if (pass.path === passDisplay.item.path)
                    return;
                if (first === null)
                    first = pass;
            }
        }
        if (first !== null) {
            passDisplay.item.path = first.path;
            passDisplay.item.jsondata = first.jsondata;
            backDisplay.item.jsondata = first.jsondata;
        }
    }

    function getPass(path) {
        // gets the pass with the given path
        for (var pass = 0; pass < passList.count; pass++) {
            if (passList.get(pass).path === path)
                return pass;
        }
        return null;
    }

    function removePass(path) {
        // removes the pass with the given path
        var pass = getPass(path);
        if (pass !== null) {
            notificator.removeNotification(passList.get(pass).path);
            passList.remove(pass);
        }
    }

    function calcPointsAndTime(pass) {
        // calculates the relevancy points of a pass and says whether we're close to target coordinates
        // gets the relevant date and time if available
        /* Lower numbers are more relevant, but -1 means "not active".
           This is because "null" is not allowed in models. */
        pass.points = -1;
        pass.iconImage = pass.iconImage || "";
        var data = JSON.parse(pass.jsondata);
        var close = false;
        var now = Date.now();
        pass.archiveKey = Timeline.archiveKey(pass.path, data);
        var state = Timeline.classify(data, now, settingsStore.hoursBefore,
                                      settingsStore.hoursAfter, settingsStore.archiveAfterHours,
                                      settingsStore.archiveState(pass.archiveKey));
        pass.eventTime = state.eventTime;
        pass.expiryTime = state.expiryTime;
        pass.archived = state.archived;
        pass.past = state.inactive;
        pass.current = settingsStore.checkTime && state.current;
        pass.timelineSection = state.section;
        pass.relevantDate = state.eventTime
                ? dateTimeFormat.format(data.relevantDate, "medium", "short", false) : "";
        if (pass.archived || state.inactive)
            return false;
        if (pass.current)
            pass.points = Math.abs(state.eventTime - now) / 1000;
        if (pass.points === -1 && settingsStore.checkDistance && "locations" in data && locator.valid && locator.position.latitudeValid && locator.position.longitudeValid) {
            // close to one of the target destinations?
            var here = locator.position.coordinate;
            try {
                for (var location = 0; location < data.locations.length; location++) {
                    var there = QtPositioning.coordinate(data.locations[location].latitude, data.locations[location].longitude);
                    var posDiff = here.distanceTo(there);  // distance in meter
                    var maxDistance = settingsStore.maxDistance;
                    if (settingsStore.overrideDistance && "maxDistance" in data)
                        maxDistance = data.maxDistance;
                    if (posDiff <= maxDistance && (pass.points === -1 || pass.points > posDiff))
                        pass.points = posDiff;
                    if (posDiff <= maxDistance + 1000)
                        close = true;  // close enough to always check GPS
                }
            }
            catch (e) {
                notificator.removeNotification(pass.path);
                return false;  // faulty pass
            }
            if (pass.points !== -1)
                pass.points += 36000; // close to target time is more relevant than close to destination
        }
        return close;
    }

    function comparePasses(a, b) {
        return Timeline.compare(a, b);
    }

    function checkPassList() {
        // recalculates all relevancy points, reorders the list and updates GPS precision
        var passes = [];
        var close = false;
        for (var pass = 0; pass < passList.count; pass++) {
            // we work with a copy
            var modelPass = passList.get(pass);
            var thisPass = {};
            for (var key in modelPass)
                thisPass[key] = modelPass[key];
            if (calcPointsAndTime(thisPass))
                close = true;
            passes.push(thisPass);
        }
        passes.sort(comparePasses);
        if (locator.precise !== close)
            locator.precise = close;
        updatePasses(passes);
    }
}
