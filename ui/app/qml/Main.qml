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

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // —— Left sidebar (Grafana / Ignition style) ——
        Rectangle {
            Layout.preferredWidth: Theme.sidebarWidth
            Layout.fillHeight: true
            color: Theme.bgSidebar

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
                    Layout.preferredHeight: 64

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spaceLg
                        anchors.rightMargin: Theme.spaceLg
                        spacing: Theme.spaceMd

                        Rectangle {
                            width: 32; height: 32; radius: Theme.radiusMd
                            color: Theme.accent
                            Label {
                                anchors.centerIn: parent
                                text: "FS"
                                font.bold: true
                                font.pixelSize: Theme.fontSm
                                color: "#ffffff"
                            }
                        }

                        ColumnLayout {
                            spacing: 1
                            Layout.fillWidth: true
                            Label {
                                text: "ForgeSight"
                                font.bold: true
                                font.pixelSize: Theme.fontLg
                                color: Theme.textPrimary
                            }
                            Label {
                                text: "Plant Monitor"
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
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 1.2
                    color: Theme.textMuted
                }

                Repeater {
                    model: [
                        { label: "Devices", index: 0 },
                        { label: "Alarms", index: 1 },
                        { label: "History", index: 2 },
                    ]
                    delegate: Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        Layout.leftMargin: Theme.spaceSm
                        Layout.rightMargin: Theme.spaceSm

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.radiusMd
                            color: root.navIndex === modelData.index
                                   ? Theme.accentSoft : "transparent"

                            Rectangle {
                                visible: root.navIndex === modelData.index
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 3
                                height: 20
                                radius: 1
                                color: Theme.accent
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.spaceLg
                                anchors.rightMargin: Theme.spaceMd
                                spacing: Theme.spaceMd

                                Label {
                                    text: modelData.label
                                    font.pixelSize: Theme.fontMd
                                    font.bold: root.navIndex === modelData.index
                                    color: root.navIndex === modelData.index
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
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: Theme.critical
                                    }
                                }
                            }

                            MouseArea {
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
                    Layout.preferredHeight: 56

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spaceLg
                        spacing: Theme.spaceSm

                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: wsClient.connected ? Theme.success : Theme.critical
                        }

                        ColumnLayout {
                            spacing: 1
                            Layout.fillWidth: true
                            Label {
                                text: wsClient.connected ? "Connected" : "Disconnected"
                                font.pixelSize: Theme.fontSm
                                font.bold: true
                                color: Theme.textPrimary
                            }
                            Label {
                                text: "ws://127.0.0.1:8081"
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
                        font.pixelSize: Theme.fontSm
                        color: root.statusHint.length > 0 ? Theme.warning : Theme.textMuted
                        Layout.fillWidth: true
                    }

                    // KPI chips — muted unless abnormal
                    Rectangle {
                        height: 28
                        width: kpiDev.implicitWidth + 20
                        radius: Theme.radiusSm
                        color: Theme.bgElevated
                        border.color: Theme.border
                        border.width: 1
                        Label {
                            id: kpiDev
                            anchors.centerIn: parent
                            text: deviceModel.count + " devices"
                            font.pixelSize: Theme.fontXs
                            color: Theme.textSecondary
                        }
                    }

                    Rectangle {
                        height: 28
                        width: kpiAlm.implicitWidth + 20
                        radius: Theme.radiusSm
                        color: alarmCount > 0 ? Theme.criticalBg : Theme.bgElevated
                        border.color: alarmCount > 0 ? Theme.critical : Theme.border
                        border.width: 1
                        Label {
                            id: kpiAlm
                            anchors.centerIn: parent
                            text: alarmCount + " unacked"
                            font.pixelSize: Theme.fontXs
                            font.bold: alarmCount > 0
                            color: alarmCount > 0 ? Theme.critical : Theme.textSecondary
                        }
                    }

                    Rectangle {
                        height: 28
                        width: kpiLive.implicitWidth + 24
                        radius: Theme.radiusSm
                        color: wsClient.connected ? Theme.successBg : Theme.criticalBg
                        border.color: wsClient.connected ? Theme.success : Theme.critical
                        border.width: 1
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            Rectangle {
                                width: 6; height: 6; radius: 3
                                color: wsClient.connected ? Theme.success : Theme.critical
                            }
                            Label {
                                id: kpiLive
                                text: wsClient.connected ? "LIVE" : "OFFLINE"
                                font.pixelSize: Theme.fontXs
                                font.bold: true
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
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontMd
                        }
                    }
                }
            }
        }
    }
}
