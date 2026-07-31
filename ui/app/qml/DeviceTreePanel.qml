import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Theme.js" as Theme

Rectangle {
    color: Theme.bgPanel

    property string searchText: ""

    // Snapshot of the plant -> floor -> device hierarchy, rebuilt from
    // deviceModel on a short timer plus whenever the device count changes.
    // Individual card values therefore refresh at most once per tick rather
    // than being bound live role-by-role — acceptable since the simulator
    // itself only publishes about once a second per sensor.
    property var groups: []

    property var collapsedPlants: []
    property var collapsedFloors: []

    function isPlantCollapsed(plant) {
        return collapsedPlants.indexOf(plant) >= 0
    }
    function togglePlant(plant) {
        var next = collapsedPlants.slice()
        var idx = next.indexOf(plant)
        if (idx >= 0) next.splice(idx, 1)
        else next.push(plant)
        collapsedPlants = next
    }
    function isFloorCollapsed(key) {
        return collapsedFloors.indexOf(key) >= 0
    }
    function toggleFloor(key) {
        var next = collapsedFloors.slice()
        var idx = next.indexOf(key)
        if (idx >= 0) next.splice(idx, 1)
        else next.push(key)
        collapsedFloors = next
    }

    function matchesSearch(dev) {
        if (searchText.length === 0) return true
        var q = searchText.toLowerCase()
        return dev.deviceId.toLowerCase().indexOf(q) >= 0
               || dev.sensor.toLowerCase().indexOf(q) >= 0
    }
    function visibleCount(devices) {
        var n = 0
        for (var i = 0; i < devices.length; i++) {
            if (matchesSearch(devices[i])) n++
        }
        return n
    }

    function rebuildGroups() {
        var result = []
        var plantNames = deviceModel.plants()
        for (var i = 0; i < plantNames.length; i++) {
            var plant = plantNames[i]
            var floorNames = deviceModel.floors(plant)
            var floorGroups = []
            for (var j = 0; j < floorNames.length; j++) {
                var floor = floorNames[j]
                floorGroups.push({ floor: floor, devices: deviceModel.devicesFor(plant, floor) })
            }
            result.push({ plant: plant, floors: floorGroups })
        }
        groups = result
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: rebuildGroups()
    }

    Connections {
        target: deviceModel
        function onCountChanged() { rebuildGroups() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        // Toolbar
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceMd

            Label {
                text: deviceModel.count + " sensors reporting"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSm
                color: Theme.textSecondary
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                height: 36
                width: 280
                radius: Theme.radiusMd
                color: Theme.bgInput
                border.color: searchInput.activeFocus ? Theme.accent : Theme.border
                border.width: searchInput.activeFocus ? 1.4 : 1
                Behavior on border.color { ColorAnimation { duration: Theme.motionFast } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceMd
                    anchors.rightMargin: Theme.spaceMd
                    spacing: Theme.spaceSm

                    Icon {
                        name: "search"
                        width: 14; height: 14
                        color: searchInput.activeFocus ? Theme.accent : Theme.textMuted
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    }

                    TextInput {
                        id: searchInput
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSm
                        Layout.fillWidth: true
                        clip: true
                        selectByMouse: true
                        onTextChanged: searchText = text

                        Label {
                            visible: searchInput.text.length === 0
                            text: "Filter devices or sensors…"
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSm
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
                            onClicked: {
                                searchInput.text = ""
                                searchText = ""
                            }
                        }
                    }
                }
            }
        }

        // Plant -> floor -> device tree
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: parent.width
                spacing: Theme.spaceLg

                Repeater {
                    model: groups

                    delegate: ColumnLayout {
                        id: plantSection
                        Layout.fillWidth: true
                        spacing: Theme.spaceMd

                        property var plantData: modelData
                        property int plantVisibleCount: {
                            var n = 0
                            for (var i = 0; i < plantData.floors.length; i++)
                                n += visibleCount(plantData.floors[i].devices)
                            return n
                        }
                        visible: plantVisibleCount > 0

                        // Plant header
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spaceSm

                            MouseArea {
                                Layout.fillWidth: true
                                implicitHeight: plantRow.implicitHeight
                                cursorShape: Qt.PointingHandCursor
                                onClicked: togglePlant(plantData.plant)

                                RowLayout {
                                    id: plantRow
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    spacing: Theme.spaceSm

                                    Icon {
                                        name: isPlantCollapsed(plantData.plant) ? "chevron-right" : "chevron-down"
                                        width: 12; height: 12
                                        color: Theme.textSecondary
                                    }
                                    Icon {
                                        name: "layers"
                                        width: 15; height: 15
                                        color: Theme.accent
                                    }
                                    Label {
                                        text: plantData.plant
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fontLg
                                        font.bold: true
                                        color: Theme.textPrimary
                                    }
                                    Label {
                                        text: plantVisibleCount + " sensors"
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fontSm
                                        color: Theme.textMuted
                                    }
                                    Item { Layout.fillWidth: true }
                                }
                            }
                        }

                        // Floors within this plant
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: Theme.spaceXl
                            spacing: Theme.spaceMd
                            visible: !isPlantCollapsed(plantData.plant)

                            Repeater {
                                model: plantData.floors

                                delegate: ColumnLayout {
                                    id: floorSection
                                    Layout.fillWidth: true
                                    spacing: Theme.spaceSm

                                    property var floorData: modelData
                                    property string floorKey: plantData.plant + "||" + floorData.floor
                                    property int floorVisibleCount: visibleCount(floorData.devices)
                                    visible: floorVisibleCount > 0

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.spaceSm

                                        MouseArea {
                                            Layout.fillWidth: true
                                            implicitHeight: floorRow.implicitHeight
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: toggleFloor(floorKey)

                                            RowLayout {
                                                id: floorRow
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                spacing: Theme.spaceSm

                                                Icon {
                                                    name: isFloorCollapsed(floorKey) ? "chevron-right" : "chevron-down"
                                                    width: 10; height: 10
                                                    color: Theme.textMuted
                                                }
                                                Label {
                                                    text: floorData.floor
                                                    font.family: Theme.fontFamily
                                                    font.pixelSize: Theme.fontMd
                                                    font.bold: true
                                                    color: Theme.textSecondary
                                                }
                                                Label {
                                                    text: floorVisibleCount
                                                    font.family: Theme.fontFamily
                                                    font.pixelSize: Theme.fontXs
                                                    color: Theme.textMuted
                                                }
                                                Item { Layout.fillWidth: true }
                                            }
                                        }
                                    }

                                    Flow {
                                        Layout.fillWidth: true
                                        spacing: Theme.spaceLg
                                        visible: !isFloorCollapsed(floorKey)

                                        Repeater {
                                            model: floorData.devices

                                            delegate: ShadowCard {
                                                id: card
                                                width: Math.min(320, Math.max(264, (floorSection.width - Theme.spaceLg) / 3 - 1))
                                                height: 172
                                                cardRadius: Theme.radiusLg
                                                color: mouseArea.containsMouse ? Theme.bgCardHover : Theme.bgCard
                                                shadowBlur: mouseArea.containsMouse ? 26 : 14
                                                shadowOffsetY: mouseArea.containsMouse ? 10 : 4
                                                glow: mouseArea.containsMouse && modelData.status !== "normal"
                                                glowColor: Theme.statusColor(modelData.status)
                                                scale: mouseArea.containsMouse ? 1.015 : 1.0

                                                Behavior on scale { NumberAnimation { duration: Theme.motionMed; easing.type: Theme.easeOut } }
                                                Behavior on shadowBlur { NumberAnimation { duration: Theme.motionMed } }
                                                Behavior on shadowOffsetY { NumberAnimation { duration: Theme.motionMed } }

                                                visible: matchesSearch(modelData)

                                                border.color: Theme.border

                                                MouseArea {
                                                    id: mouseArea
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    cursorShape: Qt.PointingHandCursor
                                                }

                                                // Left status rail — muted when normal (ISA-101)
                                                Rectangle {
                                                    anchors.left: parent.left
                                                    anchors.top: parent.top
                                                    anchors.bottom: parent.bottom
                                                    anchors.margins: 1
                                                    width: 3
                                                    radius: 1.5
                                                    color: Theme.statusColor(modelData.status)
                                                    opacity: modelData.status === "normal" ? 0.5 : 1.0
                                                }

                                                // Rolling sparkline of recent values, drawn locally
                                                // per card from the periodic group snapshots.
                                                property var history: []
                                                property real trackedValue: modelData.value
                                                onTrackedValueChanged: {
                                                    history.push(trackedValue)
                                                    if (history.length > 24) history.shift()
                                                    spark.requestPaint()
                                                }
                                                Component.onCompleted: {
                                                    history.push(trackedValue)
                                                    spark.requestPaint()
                                                }

                                                Canvas {
                                                    id: spark
                                                    anchors.left: parent.left
                                                    anchors.right: parent.right
                                                    anchors.bottom: parent.bottom
                                                    anchors.bottomMargin: 1
                                                    anchors.leftMargin: 4
                                                    anchors.rightMargin: 4
                                                    height: 40
                                                    opacity: 0.9
                                                    onPaint: {
                                                        var ctx = getContext("2d")
                                                        ctx.reset()
                                                        ctx.clearRect(0, 0, width, height)
                                                        var pts = card.history
                                                        if (pts.length < 2) return
                                                        var minV = Math.min.apply(null, pts)
                                                        var maxV = Math.max.apply(null, pts)
                                                        var range = (maxV - minV) || 1
                                                        var stepX = width / (pts.length - 1)
                                                        var col = Theme.statusColor(modelData.status)

                                                        function yFor(v) { return height - 4 - ((v - minV) / range) * (height - 8) }

                                                        ctx.beginPath()
                                                        ctx.moveTo(0, yFor(pts[0]))
                                                        for (var i = 1; i < pts.length; i++) ctx.lineTo(i * stepX, yFor(pts[i]))
                                                        ctx.lineTo((pts.length - 1) * stepX, height)
                                                        ctx.lineTo(0, height)
                                                        ctx.closePath()
                                                        var g = ctx.createLinearGradient(0, 0, 0, height)
                                                        g.addColorStop(0, col + "33")
                                                        g.addColorStop(1, col + "00")
                                                        ctx.fillStyle = g
                                                        ctx.fill()

                                                        ctx.beginPath()
                                                        ctx.moveTo(0, yFor(pts[0]))
                                                        for (var j = 1; j < pts.length; j++) ctx.lineTo(j * stepX, yFor(pts[j]))
                                                        ctx.strokeStyle = col
                                                        ctx.lineWidth = 1.6
                                                        ctx.lineJoin = "round"
                                                        ctx.lineCap = "round"
                                                        ctx.stroke()
                                                    }
                                                }

                                                ColumnLayout {
                                                    anchors.left: parent.left
                                                    anchors.right: parent.right
                                                    anchors.top: parent.top
                                                    anchors.leftMargin: Theme.spaceLg
                                                    anchors.rightMargin: Theme.spaceLg
                                                    anchors.topMargin: Theme.spaceMd
                                                    spacing: Theme.spaceSm

                                                    RowLayout {
                                                        Layout.fillWidth: true
                                                        spacing: Theme.spaceSm

                                                        Rectangle {
                                                            width: 26; height: 26; radius: Theme.radiusSm
                                                            color: Theme.statusBg(modelData.status)
                                                            Icon {
                                                                anchors.centerIn: parent
                                                                name: "gauge"
                                                                width: 14; height: 14
                                                                color: Theme.statusColor(modelData.status)
                                                            }
                                                        }

                                                        ColumnLayout {
                                                            spacing: 1
                                                            Layout.fillWidth: true
                                                            Label {
                                                                text: modelData.deviceId
                                                                font.family: Theme.fontFamily
                                                                font.pixelSize: Theme.fontMd
                                                                font.bold: true
                                                                color: Theme.textPrimary
                                                                elide: Text.ElideRight
                                                                Layout.fillWidth: true
                                                            }
                                                            Label {
                                                                text: modelData.sensor
                                                                font.family: Theme.fontFamily
                                                                font.pixelSize: Theme.fontSm
                                                                color: Theme.textSecondary
                                                            }
                                                        }

                                                        Rectangle {
                                                            height: 22
                                                            width: badgeLabel.implicitWidth + 14
                                                            radius: Theme.radiusSm
                                                            color: Theme.statusBg(modelData.status)
                                                            border.color: Theme.statusColor(modelData.status)
                                                            border.width: modelData.status === "normal" ? 0 : 1
                                                            Label {
                                                                id: badgeLabel
                                                                anchors.centerIn: parent
                                                                text: modelData.status.toUpperCase()
                                                                font.family: Theme.fontFamily
                                                                font.pixelSize: 10
                                                                font.bold: true
                                                                color: Theme.statusColor(modelData.status)
                                                            }
                                                        }
                                                    }

                                                    RowLayout {
                                                        spacing: 6
                                                        Layout.topMargin: 2
                                                        Label {
                                                            text: modelData.value.toFixed(1)
                                                            font.family: Theme.fontFamilyMono
                                                            font.pixelSize: Theme.fontDisplay
                                                            font.bold: true
                                                            color: modelData.status === "normal"
                                                                   ? Theme.textPrimary
                                                                   : Theme.statusColor(modelData.status)
                                                            Layout.alignment: Qt.AlignBaseline
                                                        }
                                                        Label {
                                                            text: modelData.unit
                                                            font.family: Theme.fontFamily
                                                            font.pixelSize: Theme.fontSm
                                                            color: Theme.textMuted
                                                            Layout.alignment: Qt.AlignBaseline
                                                            Layout.bottomMargin: 4
                                                        }
                                                        Item { Layout.fillWidth: true }
                                                    }
                                                }

                                                Label {
                                                    anchors.left: parent.left
                                                    anchors.right: parent.right
                                                    anchors.bottom: spark.top
                                                    anchors.leftMargin: Theme.spaceLg
                                                    anchors.rightMargin: Theme.spaceLg
                                                    anchors.bottomMargin: 2
                                                    text: modelData.timestamp
                                                    font.family: Theme.fontFamilyMono
                                                    font.pixelSize: 10
                                                    color: Theme.textMuted
                                                    elide: Text.ElideRight
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
    }

    // Empty state
    Column {
        anchors.centerIn: parent
        spacing: Theme.spaceMd
        visible: deviceModel.count === 0

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: "grid"
            width: 32; height: 32
            color: Theme.textMuted
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "No devices connected"
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLg
            font.bold: true
            color: Theme.textSecondary
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Start the simulator and ingestion service to see live readings."
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            color: Theme.textMuted
        }
    }
}
