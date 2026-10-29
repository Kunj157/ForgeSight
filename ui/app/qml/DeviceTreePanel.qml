import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ForgeSight.Models

Rectangle {
    id: panel
    color: "transparent"

    property string searchText: ""
    // "cards" (the live tree) or "floor" (3D / 2.5D plant overview).
    property string viewMode: "cards"
    // "<deviceId>|<sensor>" of the selected sensor, shared by both views.
    property string selectedKey: ""
    property bool failed3d: false
    readonly property bool use3d: quick3dAvailable && displayOptions.enable3d && !failed3d
                                  && GraphicsInfo.api !== GraphicsInfo.Software

    function locate(key) {
        selectedKey = key
        viewMode = "floor"
        if (floorLoader.item && floorLoader.item.focusOn)
            floorLoader.item.focusOn(key)
    }
    function showInList(key) {
        selectedKey = key
        viewMode = "cards"
        searchInput.text = key.split("|")[0]
    }
    // Bumped every few seconds so relative "updated" labels age without
    // per-card timers.
    property int tick: 0

    // Plant -> floor structure only. Cards bind to live DeviceFilterModel
    // rows, so this is rebuilt (and Repeaters reset) only when the set of
    // plants/floors actually changes — not on every reading.
    property var groups: []
    property string _signature: ""

    property var collapsedPlants: []
    property var collapsedFloors: []

    function toggle(list, key) {
        var next = list.slice()
        var idx = next.indexOf(key)
        if (idx >= 0) next.splice(idx, 1)
        else next.push(key)
        return next
    }

    function rebuildGroups() {
        var result = []
        var plantNames = deviceModel.plants()
        for (var i = 0; i < plantNames.length; i++)
            result.push({ plant: plantNames[i], floors: deviceModel.floors(plantNames[i]) })
        var sig = JSON.stringify(result)
        if (sig !== _signature) {
            _signature = sig
            groups = result
        }
    }

    readonly property int floorCount: {
        var n = 0
        for (var i = 0; i < groups.length; i++)
            n += groups[i].floors.length
        return n
    }
    readonly property real healthyPct: everything.count > 0
        ? 100 * (everything.count - everything.criticalCount) / everything.count : 100

    Timer {
        interval: 2000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: { rebuildGroups(); panel.tick++ }
    }

    Connections {
        target: deviceModel
        function onCountChanged() { rebuildGroups() }
    }

    DeviceFilterModel { id: everything; sourceDevices: deviceModel }
    DeviceFilterModel { id: matches; sourceDevices: deviceModel; search: panel.searchText }
    // Only laid out while the floor view is showing.
    PlantSceneModel { id: plantScene; sourceDevices: panel.viewMode === "floor" ? deviceModel : null }

    component Kpi: GlowCard {
        id: kpi
        property string label: ""
        property string caption: ""
        default property alias body: slot.data
        Layout.fillWidth: true
        Layout.preferredHeight: 96
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spaceLg
            spacing: 2
            Text {
                text: kpi.label
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.bold: true
                font.letterSpacing: Theme.trackingCaps
                color: Theme.textMuted
            }
            Item { id: slot; Layout.fillWidth: true; Layout.preferredHeight: 36 }
            Text {
                text: kpi.caption
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontXs
                color: Theme.textMuted
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        // —— KPI strip ——
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceLg

            Kpi {
                label: "SENSORS"
                caption: "reporting across " + panel.groups.length + (panel.groups.length === 1 ? " plant" : " plants")
                SlidingNumber { value: everything.count; font.pixelSize: 28; font.bold: true }
            }
            Kpi {
                label: "HEALTHY"
                caption: "of sensors within normal range"
                AnimatedNumber {
                    value: panel.healthyPct
                    decimals: 1
                    suffix: "%"
                    font.pixelSize: 28
                    font.bold: true
                    baseColor: panel.healthyPct < 90 ? Theme.warning : Theme.success
                }
            }
            Kpi {
                label: "CRITICAL"
                caption: everything.criticalCount > 0 ? "sensors reporting anomalies" : "no anomalies right now"
                trail: everything.criticalCount > 0
                borderColor: everything.criticalCount > 0 ? Theme.alpha(Theme.critical, 0.4) : Theme.border
                SlidingNumber {
                    value: everything.criticalCount
                    font.pixelSize: 28
                    font.bold: true
                    color: everything.criticalCount > 0 ? Theme.critical : Theme.textPrimary
                }
            }
            Kpi {
                label: "FLOORS"
                caption: "monitored production areas"
                SlidingNumber { value: panel.floorCount; font.pixelSize: 28; font.bold: true }
            }
        }

        // —— Toolbar ——
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceMd

            SegmentedControl {
                currentValue: panel.viewMode
                onActivated: function(v) { panel.viewMode = v }
                model: [
                    { label: "Cards", value: "cards" },
                    { label: "3D floor", value: "floor" },
                ]
            }

            Text {
                visible: panel.viewMode === "cards"
                text: panel.searchText.length > 0
                      ? matches.count + " of " + everything.count + " sensors match"
                      : "Plant floor"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontMd
                font.bold: true
                color: Theme.textSecondary
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: panel.viewMode === "cards"
                implicitHeight: 36
                implicitWidth: 300
                radius: Theme.radiusMd
                color: Theme.bgInput
                border.color: searchInput.activeFocus ? Theme.accent : Theme.border
                border.width: 1
                Behavior on border.color { ColorAnimation { duration: Theme.motionFast } }

                // Focus ring
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -3
                    radius: parent.radius + 3
                    color: "transparent"
                    border.width: 3
                    border.color: Theme.alpha(Theme.accent, 0.18)
                    opacity: searchInput.activeFocus ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceMd
                    anchors.rightMargin: Theme.spaceMd
                    spacing: Theme.spaceSm

                    Icon {
                        name: "search"
                        width: 14; height: 14
                        color: searchInput.activeFocus ? Theme.accent : Theme.textMuted
                    }
                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSm
                        clip: true
                        selectByMouse: true
                        onTextChanged: panel.searchText = text

                        Text {
                            visible: searchInput.text.length === 0
                            text: "Filter devices or sensors…"
                            font: searchInput.font
                            color: Theme.textMuted
                        }
                    }
                    Icon {
                        name: "close"
                        width: 12; height: 12
                        color: Theme.textMuted
                        visible: searchInput.text.length > 0
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -6
                            cursorShape: Qt.PointingHandCursor
                            onClicked: searchInput.text = ""
                        }
                    }
                }
            }
        }

        // —— Plant -> floor -> device tree ——
        ScrollView {
            id: scroller
            visible: panel.viewMode === "cards"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                width: scroller.availableWidth
                spacing: Theme.spaceXl

                Repeater {
                    model: panel.groups

                    delegate: Column {
                        id: plantSection
                        width: parent.width
                        spacing: Theme.spaceMd

                        readonly property string plant: modelData.plant
                        readonly property var floors: modelData.floors
                        readonly property bool collapsed: panel.collapsedPlants.indexOf(plant) >= 0

                        DeviceFilterModel {
                            id: plantDevices
                            sourceDevices: deviceModel
                            plant: plantSection.plant
                            search: panel.searchText
                        }
                        visible: plantDevices.count > 0

                        // Plant header
                        Item {
                            width: parent.width
                            height: 32
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: panel.collapsedPlants = panel.toggle(panel.collapsedPlants, plantSection.plant)
                            }
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: Theme.spaceSm
                                Icon {
                                    name: "chevron-down"
                                    width: 12; height: 12
                                    color: Theme.textSecondary
                                    rotation: plantSection.collapsed ? -90 : 0
                                    Behavior on rotation { NumberAnimation { duration: Theme.motionMed; easing.type: Easing.OutCubic } }
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Icon {
                                    name: "layers"
                                    width: 15; height: 15
                                    color: Theme.accent
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    text: plantSection.plant
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontLg
                                    font.bold: true
                                    color: Theme.textPrimary
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    text: plantDevices.count + " sensors"
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSm
                                    color: Theme.textMuted
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                StatusPill {
                                    visible: plantDevices.criticalCount > 0
                                    status: "critical"
                                    pulse: true
                                    text: plantDevices.criticalCount + " critical"
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 1
                                color: Theme.divider
                            }
                        }

                        // Floors
                        Column {
                            width: parent.width
                            spacing: Theme.spaceLg
                            visible: !plantSection.collapsed

                            Repeater {
                                model: plantSection.floors

                                delegate: Column {
                                    id: floorSection
                                    width: parent.width
                                    spacing: Theme.spaceSm

                                    readonly property string floor: modelData
                                    readonly property string key: plantSection.plant + "||" + floor
                                    readonly property bool collapsed: panel.collapsedFloors.indexOf(key) >= 0

                                    DeviceFilterModel {
                                        id: floorDevices
                                        sourceDevices: deviceModel
                                        plant: plantSection.plant
                                        floor: floorSection.floor
                                        search: panel.searchText
                                    }
                                    visible: floorDevices.count > 0

                                    Item {
                                        width: parent.width
                                        height: 24
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: panel.collapsedFloors = panel.toggle(panel.collapsedFloors, floorSection.key)
                                        }
                                        Row {
                                            anchors.verticalCenter: parent.verticalCenter
                                            leftPadding: Theme.spaceXs
                                            spacing: Theme.spaceSm
                                            Icon {
                                                name: "chevron-down"
                                                width: 10; height: 10
                                                color: Theme.textMuted
                                                rotation: floorSection.collapsed ? -90 : 0
                                                Behavior on rotation { NumberAnimation { duration: Theme.motionMed; easing.type: Easing.OutCubic } }
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                            Text {
                                                text: floorSection.floor
                                                font.family: Theme.fontFamily
                                                font.pixelSize: Theme.fontMd
                                                font.bold: true
                                                color: Theme.textSecondary
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                            Rectangle {
                                                implicitWidth: floorCountLabel.implicitWidth + 12
                                                implicitHeight: 18
                                                radius: 9
                                                color: Theme.bgElevated
                                                anchors.verticalCenter: parent.verticalCenter
                                                Text {
                                                    id: floorCountLabel
                                                    anchors.centerIn: parent
                                                    text: floorDevices.count
                                                    font.family: Theme.fontFamilyMono
                                                    font.pixelSize: 10
                                                    color: Theme.textSecondary
                                                }
                                            }
                                        }
                                    }

                                    Flow {
                                        id: grid
                                        width: parent.width
                                        spacing: Theme.spaceLg
                                        visible: !floorSection.collapsed

                                        // Responsive columns: as many >= 260px cards as fit.
                                        readonly property int columns: Math.max(1, Math.floor((width + spacing) / (260 + spacing)))
                                        readonly property real cardWidth: (width - (columns - 1) * spacing) / columns

                                        Repeater {
                                            model: floorDevices
                                            delegate: DeviceCard {
                                                width: grid.cardWidth
                                                height: 184
                                                deviceId: model.deviceId
                                                sensor: model.sensor
                                                value: model.value
                                                unit: model.unit
                                                status: model.status
                                                timestamp: model.timestamp
                                                tick: panel.tick
                                                revealIndex: index
                                                selected: panel.selectedKey === model.deviceId + "|" + model.sensor
                                                onActivated: panel.selectedKey = model.deviceId + "|" + model.sensor
                                                onLocateRequested: panel.locate(model.deviceId + "|" + model.sensor)
                                                inView: {
                                                    scroller.contentItem.contentY
                                                    panel.tick
                                                    y + floorSection.y + plantSection.y
                                                    var p = mapToItem(scroller, 0, 0)
                                                    return p.y + height > -200 && p.y < scroller.height + 200
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // —— Plant floor (3D, or 2.5D fallback) ——
        GlowCard {
            id: floorCard
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: panel.viewMode === "floor"
            interactive: false
            clip: true

            Loader {
                id: floorLoader
                anchors.fill: parent
                anchors.margins: 1
                active: panel.viewMode === "floor"
                source: panel.use3d ? "PlantView3D.qml" : "PlantView25D.qml"
                onStatusChanged: {
                    if (status === Loader.Error && panel.use3d) {
                        console.warn("3D plant view unavailable — using the 2.5D view")
                        panel.failed3d = true
                    }
                }
                onLoaded: {
                    item.scene = plantScene
                    item.selectedKey = Qt.binding(function() { return panel.selectedKey })
                    item.picked.connect(function(key) { panel.selectedKey = key })
                    if (panel.selectedKey.length > 0 && item.focusOn)
                        item.focusOn(panel.selectedKey)
                }
            }

            // Legend + renderer badge
            Row {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: Theme.spaceMd
                spacing: Theme.spaceSm
                StatusPill { status: "info"; showDot: false; text: panel.use3d ? "Qt Quick 3D · instanced" : "2.5D view" }
                StatusPill { status: "normal"; text: "height = live value" }
                StatusPill { status: "critical"; pulse: true; text: plantScene.criticalCount + " critical" }
            }
            Text {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: Theme.spaceMd
                text: panel.use3d ? "Drag to orbit · scroll to zoom · click a pillar" : "Drag to rotate · click a pillar"
                font.family: Theme.fontFamily
                font.pixelSize: 10
                color: Theme.textMuted
            }

            // Selected device details
            GlowCard {
                id: detail
                readonly property int row: plantScene.deviceCount >= 0 && panel.selectedKey.length > 0
                                           ? plantScene.indexOfKey(panel.selectedKey) : -1
                readonly property var dev: { plantScene.deviceCount; return row >= 0 ? plantScene.devices()[row] : null }
                visible: dev !== null
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.spaceMd
                width: 250
                height: 150
                elevation: 2
                interactive: false
                trail: dev !== null && dev.critical
                Column {
                    anchors.fill: parent
                    anchors.margins: Theme.spaceLg
                    spacing: 6
                    Row {
                        spacing: Theme.spaceSm
                        Text {
                            text: detail.dev ? detail.dev.deviceId : ""
                            font.family: Theme.fontFamily; font.pixelSize: Theme.fontMd; font.bold: true
                            color: Theme.textPrimary
                        }
                        StatusPill { status: detail.dev && detail.dev.critical ? "critical" : "normal"; pulse: detail.dev && detail.dev.critical }
                    }
                    Text {
                        text: detail.dev ? detail.dev.sensor + " · " + detail.dev.plant + " / " + detail.dev.floor : ""
                        font.family: Theme.fontFamily; font.pixelSize: Theme.fontXs; color: Theme.textMuted
                    }
                    AnimatedNumber {
                        value: detail.dev ? detail.dev.value : 0
                        font.pixelSize: Theme.fontDisplay; font.bold: true
                        baseColor: detail.dev && detail.dev.critical ? Theme.critical : Theme.textPrimary
                    }
                    Row {
                        spacing: Theme.spaceSm
                        FxButton { text: "Show card"; icon: "grid"; compact: true; onClicked: panel.showInList(panel.selectedKey) }
                        FxButton { text: "Clear"; icon: "close"; compact: true; onClicked: panel.selectedKey = "" }
                    }
                }
            }
        }
    }

    EmptyState {
        anchors.centerIn: parent
        visible: deviceModel.count === 0
        icon: "grid"
        title: "No devices connected"
        subtitle: "Start the simulator and ingestion service to see live readings."
    }

    EmptyState {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 80
        visible: deviceModel.count > 0 && matches.count === 0
        icon: "search"
        title: "No sensors match “" + panel.searchText + "”"
        subtitle: "Try a device id such as pump-001, or a sensor such as temperature."
        actionText: "Clear filter"
        onAction: searchInput.text = ""
    }
}
