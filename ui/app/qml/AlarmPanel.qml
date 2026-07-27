import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#0f0f17"

    readonly property color surface0: "#1a1a2e"
    readonly property color surface1: "#232340"
    readonly property color surface2: "#2d2d50"
    readonly property color surface3: "#3d3d6b"
    readonly property color text: "#e0e0f0"
    readonly property color subtext: "#9090b0"
    readonly property color muted: "#505070"
    readonly property color blue: "#6c8cff"
    readonly property color green: "#4ade80"
    readonly property color red: "#f87171"
    readonly property color yellow: "#fbbf24"
    readonly property color accent: "#818cf8"
    readonly property color cardBg: "#14142a"
    readonly property color cardBorder: "#2a2a45"

    property string filterSeverity: "all"


    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 28
        anchors.rightMargin: 28
        anchors.topMargin: 24
        anchors.bottomMargin: 20
        spacing: 20

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Label {
                text: "Alarms"
                font.pixelSize: 22
                font.bold: true
                color: text
            }

            Rectangle {
                height: 28
                width: countLabel.width + 20
                radius: 8
                color: alarmModel.unacknowledgedCount > 0 ? "#2e0a0a" : surface0
                border.color: alarmModel.unacknowledgedCount > 0 ? red : surface2
                border.width: 1
                Label {
                    id: countLabel
                    anchors.centerIn: parent
                    text: alarmModel.unacknowledgedCount + " unacknowledged"
                    font.pixelSize: 12
                    font.bold: true
                    color: alarmModel.unacknowledgedCount > 0 ? red : muted
                }
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                spacing: 6
                Repeater {
                    model: [
                        { label: "All", sev: "all", color: accent },
                        { label: "Critical", sev: "critical", color: red },
                        { label: "Warning", sev: "warning", color: yellow },
                        { label: "Info", sev: "info", color: blue },
                    ]
                    delegate: Rectangle {
                        height: 30
                        width: filterLabel.width + 20
                        radius: 8
                        color: filterSeverity === modelData.sev ? surface2 : surface0
                        border.color: filterSeverity === modelData.sev ? modelData.color : surface2
                        border.width: filterSeverity === modelData.sev ? 1 : 0
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Label {
                            id: filterLabel
                            anchors.centerIn: parent
                            text: modelData.label
                            font.pixelSize: 12
                            font.bold: filterSeverity === modelData.sev
                            color: filterSeverity === modelData.sev ? modelData.color : subtext
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: filterSeverity = modelData.sev
                        }
                    }
                }
            }

            Rectangle {
                height: 30; width: 80; radius: 8
                color: surface0; border.color: surface2; border.width: 1
                Label {
                    anchors.centerIn: parent
                    text: "Clear All"
                    font.pixelSize: 12
                    color: subtext
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onEntered: parent.color = surface2
                    onExited: parent.color = surface0
                    onClicked: alarmModel.clear()
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.interactive: true

            ListView {
                id: alarmList
                model: alarmModel
                spacing: 10
                delegate: alarmDelegate
                section.property: "severity"
                section.delegate: Rectangle {
                    width: alarmList.width; height: 1; color: surface2
                }
            }
        }
    }

    Component {
        id: alarmDelegate
        Rectangle {
            id: alarmCard
            width: alarmList.width
            height: acknowledged ? 88 : 112
            radius: 10
            color: cardBg
            border.color: severity === "critical" ? "#4a1a1a"
                         : severity === "warning" ? "#4a4a1a"
                         : severity === "info" ? "#1a2a4a"
                         : cardBorder
            border.width: acknowledged ? 1 : 1
            clip: true

            Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

            opacity: {
                if (filterSeverity === "all") return 1.0
                return severity === filterSeverity ? 1.0 : 0.2
            }
            visible: opacity > 0.15
            Behavior on opacity { NumberAnimation { duration: 200 } }

            Rectangle {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: 4
                color: severity === "critical" ? red
                     : severity === "warning" ? yellow
                     : blue
                anchors.topMargin: 6
                anchors.bottomMargin: 6
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                ColumnLayout {
                    spacing: 4
                    Layout.fillWidth: true

                    RowLayout {
                        spacing: 8
                        Label {
                            text: deviceId
                            font.pixelSize: 15
                            font.bold: true
                            color: acknowledged ? subtext : text
                        }
                        Label {
                            text: "\u00B7 " + sensor
                            font.pixelSize: 12
                            color: acknowledged ? muted : subtext
                        }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            height: 22
                            width: sevLabel.width + 12
                            radius: 5
                            color: severity === "critical" ? "#3a1a1a"
                                 : severity === "warning" ? "#3a3a1a"
                                 : "#0a1a2e"
                            visible: !acknowledged
                            Label {
                                id: sevLabel
                                anchors.centerIn: parent
                                text: severity.toUpperCase()
                                font.pixelSize: 10
                                font.bold: true
                                color: severity === "critical" ? red
                                     : severity === "warning" ? yellow
                                     : blue
                            }
                        }
                    }

                    Label {
                        text: message
                        font.pixelSize: 13
                        color: acknowledged ? muted : subtext
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.maximumHeight: 36
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        spacing: 12
                        Rectangle {
                            height: 22
                            width: valLabel.width + 12
                            radius: 5
                            color: surface0
                            border.color: surface2
                            border.width: 1
                            visible: !acknowledged
                            Label {
                                id: valLabel
                                anchors.centerIn: parent
                                text: value.toFixed(1)
                                font.pixelSize: 11
                                font.bold: true
                                color: severity === "critical" ? red
                                     : severity === "warning" ? yellow
                                     : blue
                            }
                        }
                        Label {
                            text: timestamp
                            font.pixelSize: 11
                            color: muted
                        }
                    }
                }

                Rectangle {
                    width: 80; height: 30; radius: 8
                    color: acknowledged ? "transparent" : "#1a3a1a"
                    border.color: acknowledged ? green : green
                    border.width: acknowledged ? 1 : 0
                    visible: !acknowledged
                    opacity: ackMouse.containsMouse ? 0.85 : 1.0
                    Behavior on opacity { NumberAnimation { duration: 100 } }
                    Label {
                        anchors.centerIn: parent
                        text: acknowledged ? "ACK'D" : "Acknowledge"
                        font.pixelSize: 11
                        font.bold: true
                        color: green
                    }
                    MouseArea {
                        id: ackMouse
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: alarmModel.acknowledge(model.id)
                    }
                }

                Label {
                    text: "\u2713"
                    font.pixelSize: 18
                    color: green
                    visible: acknowledged
                }
            }
        }
    }

    Label {
        anchors.centerIn: parent
        visible: alarmModel.rowCount === 0
        text: "\u2705 No active alarms"
        color: muted
        font.pixelSize: 16
    }
}
