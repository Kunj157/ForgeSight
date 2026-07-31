import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "Theme.js" as Theme

Window {
    id: root
    width: 1440
    height: 900
    visible: true
    title: "ForgeSight"
    color: Theme.bgApp
    minimumWidth: 1100
    minimumHeight: 680

    property int alarmCount: 0
    property int navIndex: 0
    property string statusHint: "Connecting…"

    Connections {
        target: alarmModel
        function onAlarmAdded() { alarmCount = alarmModel.unacknowledgedCount }
        function onAlarmAcknowledged() { alarmCount = alarmModel.unacknowledgedCount }
    }

    Connections {
        target: wsClient
        function onReadingReceived(json) {
            var obj = JSON.parse(json)
            deviceModel.updateDevice(
                obj.device_id, obj.sensor, obj.value,
                obj.unit, obj.timestamp, obj.anomaly
            )
        }
        function onAlarmReceived(json) {
            var obj = JSON.parse(json)
            alarmModel.add_alarm(
                obj.id, obj.device_id, obj.sensor,
                obj.value, obj.severity, obj.message, obj.timestamp,
                !!obj.acknowledged
            )
        }
        function onConnectedChanged() {
            if (wsClient.connected)
                statusHint = ""
            else
                statusHint = "Disconnected — run scripts/dev-up.sh"
        }
    }

    // Ambient glow in the top-left corner — the kind of soft color wash
    // Linear/Vercel dashboards use to keep a dark UI from feeling flat.
    Canvas {
        anchors.fill: parent
        z: -1
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var g = ctx.createRadialGradient(width * 0.12, 0, 0, width * 0.12, 0, width * 0.55)
            g.addColorStop(0, "#1a2a5533")
            g.addColorStop(1, "#00000000")
            ctx.fillStyle = g
            ctx.fillRect(0, 0, width, height)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // —— Left sidebar ——
        Rectangle {
            Layout.preferredWidth: Theme.sidebarWidth
            Layout.fillHeight: true

            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: Theme.bgSidebar }
                GradientStop { position: 1.0; color: Theme.bgSidebarBottom }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 0

                // Brand
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 68

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spaceLg
                        anchors.rightMargin: Theme.spaceLg
                        spacing: Theme.spaceMd

                        Rectangle {
                            width: 34; height: 34; radius: Theme.radiusMd
                            gradient: Gradient {
                                orientation: Gradient.Vertical
                                GradientStop { position: 0.0; color: Theme.accent }
                                GradientStop { position: 1.0; color: Theme.accent2 }
                            }
                            Icon {
                                anchors.centerIn: parent
                                width: 18; height: 18
                                name: "layers"
                                color: "#ffffff"
                                strokeWidth: 1.7
                            }
                        }

                        ColumnLayout {
                            spacing: 1
                            Layout.fillWidth: true
                            Label {
                                text: "ForgeSight"
                                font.family: Theme.fontFamily
                                font.bold: true
                                font.pixelSize: Theme.fontLg
                                color: Theme.textPrimary
                            }
                            Label {
                                text: "Plant Monitor"
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontXs
                                color: Theme.textMuted
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.divider
                }

                // Nav section label
                Label {
                    Layout.leftMargin: Theme.spaceLg
                    Layout.topMargin: Theme.spaceLg
                    Layout.bottomMargin: Theme.spaceSm
                    text: "MONITORING"
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 1.4
                    color: Theme.textMuted
                }

                Repeater {
                    model: [
                        { label: "Devices", index: 0, icon: "grid" },
                        { label: "Alarms", index: 1, icon: "bell" },
                        { label: "History", index: 2, icon: "chart" },
                    ]
                    delegate: Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        Layout.leftMargin: Theme.spaceSm
                        Layout.rightMargin: Theme.spaceSm

                        property bool active: root.navIndex === modelData.index

                        Rectangle {
                            id: navBg
                            anchors.fill: parent
                            radius: Theme.radiusMd
                            color: parent.active ? Theme.accentSoft
                                   : (navMouse.containsMouse ? Theme.bgCardHover : "transparent")
                            border.width: parent.active ? 1 : 0
                            border.color: Qt.rgba(0.36, 0.55, 1.0, 0.35)

                            Behavior on color { ColorAnimation { duration: Theme.motionFast } }

                            Rectangle {
                                visible: parent.parent.active
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: -1
                                width: 3
                                height: 20
                                radius: 1.5
                                gradient: Gradient {
                                    orientation: Gradient.Vertical
                                    GradientStop { position: 0.0; color: Theme.accent }
                                    GradientStop { position: 1.0; color: Theme.accent2 }
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.spaceLg
                                anchors.rightMargin: Theme.spaceMd
                                spacing: Theme.spaceMd

                                Icon {
                                    name: modelData.icon
                                    width: 16; height: 16
                                    color: parent.parent.parent.active ? Theme.accent : Theme.textMuted
                                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                                }

                                Label {
                                    text: modelData.label
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontMd
                                    font.bold: parent.parent.parent.active
                                    color: parent.parent.parent.active
                                           ? Theme.textPrimary : Theme.textSecondary
                                    Layout.fillWidth: true
                                }

                                Rectangle {
                                    visible: modelData.index === 0 && deviceModel.count > 0
                                    width: Math.max(22, countDev.implicitWidth + 10)
                                    height: 20
                                    radius: Theme.radiusSm
                                    color: Theme.bgElevated
                                    Label {
                                        id: countDev
                                        anchors.centerIn: parent
                                        text: deviceModel.count
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: Theme.textSecondary
                                    }
                                }

                                Rectangle {
                                    visible: modelData.index === 1 && alarmCount > 0
                                    width: Math.max(22, countAlm.implicitWidth + 10)
                                    height: 20
                                    radius: Theme.radiusSm
                                    color: Theme.criticalBg
                                    border.color: Theme.critical
                                    border.width: 1
                                    Label {
                                        id: countAlm
                                        anchors.centerIn: parent
                                        text: alarmCount
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: Theme.critical
                                    }
                                }
                            }

                            MouseArea {
                                id: navMouse
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                hoverEnabled: true
                                onClicked: root.navIndex = modelData.index
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // Connection footer
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.divider
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spaceLg
                        spacing: Theme.spaceSm

                        Item {
                            width: 10; height: 10
                            Rectangle {
                                anchors.centerIn: parent
                                width: 10; height: 10; radius: 5
                                color: wsClient.connected ? Theme.success : Theme.critical
                            }
                            Rectangle {
                                anchors.centerIn: parent
                                width: 10; height: 10; radius: 5
                                color: wsClient.connected ? Theme.success : Theme.critical
                                visible: wsClient.connected
                                opacity: 0.6
                                SequentialAnimation on scale {
                                    running: wsClient.connected
                                    loops: Animation.Infinite
                                    NumberAnimation { from: 1.0; to: 2.4; duration: 1400; easing.type: Easing.OutCubic }
                                }
                                SequentialAnimation on opacity {
                                    running: wsClient.connected
                                    loops: Animation.Infinite
                                    NumberAnimation { from: 0.6; to: 0.0; duration: 1400; easing.type: Easing.OutCubic }
                                }
                            }
                        }

                        ColumnLayout {
                            spacing: 1
                            Layout.fillWidth: true
                            Label {
                                text: wsClient.connected ? "Connected" : "Disconnected"
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSm
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Label {
                                text: "ws://127.0.0.1:8081"
                                font.family: Theme.fontFamilyMono
                                font.pixelSize: 10
                                color: Theme.textMuted
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }

        // —— Main content ——
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Top status bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.topBarHeight
                color: Theme.bgTopBar

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.border
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceXl
                    anchors.rightMargin: Theme.spaceXl
                    spacing: Theme.spaceLg

                    Label {
                        text: root.navIndex === 0 ? "Devices"
                            : root.navIndex === 1 ? "Alarms"
                            : "History"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXl
                        font.bold: true
                        color: Theme.textPrimary
                    }

                    Label {
                        text: root.statusHint.length > 0
                              ? root.statusHint
                              : (root.navIndex === 0
                                 ? "Live sensor readings across the plant"
                                 : root.navIndex === 1
                                 ? "Active and acknowledged alarm events"
                                 : "Historical trends and export")
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSm
                        color: root.statusHint.length > 0 ? Theme.warning : Theme.textMuted
                        Layout.fillWidth: true
                    }

                    // KPI chips — muted unless abnormal
                    Rectangle {
                        height: 30
                        width: kpiDevRow.implicitWidth + 22
                        radius: Theme.radiusMd
                        color: Theme.bgElevated
                        border.color: Theme.border
                        border.width: 1
                        RowLayout {
                            id: kpiDevRow
                            anchors.centerIn: parent
                            spacing: 6
                            Icon { name: "grid"; width: 12; height: 12; color: Theme.textSecondary }
                            Label {
                                text: deviceModel.count + " devices"
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontXs
                                color: Theme.textSecondary
                            }
                        }
                    }

                    Rectangle {
                        height: 30
                        width: kpiAlmRow.implicitWidth + 22
                        radius: Theme.radiusMd
                        color: alarmCount > 0 ? Theme.criticalBg : Theme.bgElevated
                        border.color: alarmCount > 0 ? Theme.critical : Theme.border
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: Theme.motionMed } }
                        RowLayout {
                            id: kpiAlmRow
                            anchors.centerIn: parent
                            spacing: 6
                            Icon { name: "bell"; width: 12; height: 12; color: alarmCount > 0 ? Theme.critical : Theme.textSecondary }
                            Label {
                                text: alarmCount + " unacked"
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontXs
                                font.bold: alarmCount > 0
                                color: alarmCount > 0 ? Theme.critical : Theme.textSecondary
                            }
                        }
                    }

                    Rectangle {
                        height: 30
                        width: kpiLiveRow.implicitWidth + 24
                        radius: Theme.radiusMd
                        color: wsClient.connected ? Theme.successBg : Theme.criticalBg
                        border.color: wsClient.connected ? Theme.success : Theme.critical
                        border.width: 1
                        RowLayout {
                            id: kpiLiveRow
                            anchors.centerIn: parent
                            spacing: 6
                            Rectangle {
                                width: 6; height: 6; radius: 3
                                color: wsClient.connected ? Theme.success : Theme.critical
                            }
                            Label {
                                text: wsClient.connected ? "LIVE" : "OFFLINE"
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontXs
                                font.bold: true
                                font.letterSpacing: 0.6
                                color: wsClient.connected ? Theme.success : Theme.critical
                            }
                        }
                    }
                }
            }

            // Lazy Loaders — ChartView in History is expensive; don't build
            // all tabs at startup or the window appears to hang with no logs.
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                Loader {
                    anchors.fill: parent
                    active: root.navIndex === 0 || item !== null
                    visible: root.navIndex === 0
                    source: "DeviceTreePanel.qml"
                }
                Loader {
                    anchors.fill: parent
                    active: root.navIndex === 1 || item !== null
                    visible: root.navIndex === 1
                    source: "AlarmPanel.qml"
                }
                Loader {
                    anchors.fill: parent
                    active: root.navIndex === 2
                    visible: root.navIndex === 2
                    asynchronous: true
                    source: "HistoryPanel.qml"

                    Rectangle {
                        anchors.fill: parent
                        z: 1
                        visible: parent.status === Loader.Loading
                        color: Theme.bgPanel
                        Label {
                            anchors.centerIn: parent
                            text: "Loading history view…"
                            font.family: Theme.fontFamily
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontMd
                        }
                    }
                }
            }
        }
    }
}
