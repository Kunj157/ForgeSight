import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Window {
    width: 1366
    height: 860
    visible: true
    title: "ForgeSight — Factory Pulse Monitor"
    color: "#0f0f17"
    minimumWidth: 1024
    minimumHeight: 600

    readonly property color surface0: "#1a1a2e"
    readonly property color surface1: "#232340"
    readonly property color surface2: "#2d2d50"
    readonly property color text: "#e0e0f0"
    readonly property color subtext: "#9090b0"
    readonly property color muted: "#505070"
    readonly property color blue: "#6c8cff"
    readonly property color blueAlt: "#4a6cf7"
    readonly property color green: "#4ade80"
    readonly property color red: "#f87171"
    readonly property color yellow: "#fbbf24"
    readonly property color orange: "#fb923c"
    readonly property color accent: "#818cf8"
    readonly property color accentGlow: "#818cf830"

    property int alarmCount: 0

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
            alarmModel.addAlarm(
                obj.id, obj.device_id, obj.sensor,
                obj.value, obj.severity, obj.message, obj.timestamp
            )
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 64
            color: surface0
            z: 10

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 28
                anchors.rightMargin: 28
                spacing: 16

                RowLayout {
                    spacing: 12
                    Rectangle {
                        width: 36; height: 36; radius: 10
                        color: accent
                        Label {
                            anchors.centerIn: parent
                            text: "F"
                            font.bold: true
                            font.pixelSize: 20
                            color: "#fff"
                        }
                    }
                    ColumnLayout {
                        spacing: 0
                        Label {
                            text: "ForgeSight"
                            font.bold: true
                            font.pixelSize: 18
                            font.letterSpacing: 0.5
                            color: text
                        }
                        Label {
                            text: "Factory Pulse Monitor"
                            font.pixelSize: 11
                            color: muted
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    height: 36
                    width: 180
                    radius: 8
                    color: surface1
                    border.color: surface2
                    border.width: 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 6
                        Label { text: "\u2315"; color: muted; font.pixelSize: 14 }
                        Label {
                            text: "Search devices..."
                            color: muted; font.pixelSize: 12
                            Layout.fillWidth: true
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.IBeamCursor
                    }
                }

                Item { width: 16 }

                Rectangle {
                    height: 36
                    width: connBadge.width + 28
                    radius: 8
                    color: wsClient.connected ? "#0a2e1a" : "#2e0a0a"
                    border.color: wsClient.connected ? green : red
                    border.width: 1
                    RowLayout {
                        id: connBadge
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: wsClient.connected ? green : red
                            opacity: wsClient.connected ? 1.0 : 0.6
                        }
                        Label {
                            text: wsClient.connected ? "Live" : "Offline"
                            color: wsClient.connected ? green : red
                            font.pixelSize: 12; font.bold: true
                        }
                    }
                }

                Rectangle {
                    height: 36
                    width: alarmBadge.width + 28
                    radius: 8
                    visible: alarmCount > 0
                    color: "#2e0a0a"
                    border.color: red
                    border.width: 1
                    RowLayout {
                        id: alarmBadge
                        anchors.centerIn: parent
                        spacing: 6
                        Label { text: "\u26A0"; color: red; font.pixelSize: 13 }
                        Label {
                            text: alarmCount + " alarm" + (alarmCount > 1 ? "s" : "")
                            color: red; font.pixelSize: 12; font.bold: true
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 1; color: surface2
        }

        Rectangle {
            Layout.fillWidth: true; height: 48; color: surface0
            z: 9
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 28; anchors.rightMargin: 28
                spacing: 0

                Repeater {
                    model: [
                        { icon: "\u25A3", label: "Devices", badge: deviceModel.rowCount },
                        { icon: "\u26A0", label: "Alarms", badge: alarmModel.rowCount },
                        { icon: "\u25B3", label: "History", badge: 0 },
                    ]
                    delegate: Item {
                        width: 140; height: 48
                        Rectangle {
                            anchors.fill: parent
                            color: tabBar.currentIndex === index ? surface1 : "transparent"
                            radius: 0
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left; anchors.right: parent.right
                                height: 2
                                color: tabBar.currentIndex === index ? accent : "transparent"
                            }
                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 8
                                Label {
                                    text: modelData.icon
                                    color: tabBar.currentIndex === index ? accent : muted
                                    font.pixelSize: 16
                                }
                                Label {
                                    text: modelData.label
                                    color: tabBar.currentIndex === index ? text : subtext
                                    font.pixelSize: 13
                                    font.bold: tabBar.currentIndex === index
                                }
                                Rectangle {
                                    width: badgeText.width + 12; height: 20; radius: 10
                                    visible: modelData.badge > 0
                                    color: index === 1 ? "#4a1a1a" : "#1a2e4a"
                                    Label {
                                        id: badgeText
                                        anchors.centerIn: parent
                                        text: modelData.badge
                                        color: index === 1 ? red : blue
                                        font.pixelSize: 11; font.bold: true
                                    }
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: parent.color = Qt.rgba(0.14, 0.14, 0.25, 0.5)
                                onExited: parent.color = "transparent"
                                onClicked: tabBar.currentIndex = index
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 1; color: surface2
        }

        StackLayout {
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            DeviceTreePanel {}
            AlarmPanel {}
            HistoryPanel {}
        }
    }

    TabBar {
        id: tabBar
        visible: false
        TabButton { }
        TabButton { }
        TabButton { }
    }
}
