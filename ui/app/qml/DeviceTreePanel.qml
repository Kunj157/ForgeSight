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
    readonly property color orange: "#fb923c"
    readonly property color purple: "#a78bfa"
    readonly property color teal: "#2dd4bf"
    readonly property color accent: "#818cf8"
    readonly property color cardBg: "#14142a"
    readonly property color cardBorder: "#2a2a45"

    property string searchText: ""

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
                text: "Devices"
                font.pixelSize: 22
                font.bold: true
                color: text
            }
            Label {
                text: deviceModel.rowCount + " online"
                font.pixelSize: 13
                color: green
                Layout.alignment: Qt.AlignBaseline
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                height: 36; width: 220; radius: 8
                color: surface0
                border.color: searchText.length > 0 ? accent : surface2
                border.width: 1
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10; anchors.rightMargin: 10
                    spacing: 8
                    Label { text: "\u2315"; color: searchText.length > 0 ? accent : muted; font.pixelSize: 14 }
                    TextInput {
                        id: searchInput
                        color: text
                        font.pixelSize: 12
                        Layout.fillWidth: true
                        clip: true
                        onTextChanged: searchText = text
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    Label {
                        text: "\u2715"
                        color: muted
                        font.pixelSize: 12
                        visible: searchInput.text.length > 0
                        MouseArea {
                            anchors.fill: parent
                            onClicked: { searchInput.text = ""; searchText = "" }
                        }
                    }
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.interactive: true

            Flow {
                width: parent.width
                spacing: 14

                Repeater {
                    model: deviceModel

                    delegate: Rectangle {
                        id: cardRoot
                        width: Math.min(360, parent.width * 0.5 - 7)
                        height: 164
                        radius: 12
                        color: cardBg
                        border.color: mouseArea.containsMouse ? accent : cardBorder
                        border.width: mouseArea.containsMouse ? 1 : 1
                        opacity: {
                            if (searchText.length === 0) return 1.0
                            return deviceId.toLowerCase().includes(searchText.toLowerCase())
                                   || sensor.toLowerCase().includes(searchText.toLowerCase())
                                   ? 1.0 : 0.25
                        }
                        scale: mouseArea.containsMouse ? 1.02 : 1.0
                        visible: opacity > 0.3 || searchText.length === 0

                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                        Behavior on opacity { NumberAnimation { duration: 200 } }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                        }

                        Rectangle {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            width: 4
                            height: parent.height
                            radius: 2
                            color: status === "critical" ? red
                                 : status === "warning" ? yellow
                                 : green
                            anchors.topMargin: 8
                            anchors.leftMargin: 0
                            anchors.bottomMargin: 8
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 0

                            RowLayout {
                                spacing: 10
                                Layout.fillWidth: true

                                Rectangle {
                                    width: 40; height: 40; radius: 10
                                    color: status === "critical" ? "#3a1a1a"
                                         : status === "warning" ? "#3a3a1a"
                                         : "#0a2e1a"
                                    Label {
                                        anchors.centerIn: parent
                                        text: deviceId.charAt(0).toUpperCase()
                                        font.bold: true
                                        font.pixelSize: 18
                                        color: status === "critical" ? red
                                             : status === "warning" ? yellow
                                             : green
                                    }
                                }

                                ColumnLayout {
                                    spacing: 2
                                    Layout.fillWidth: true
                                    Label {
                                        text: deviceId
                                        font.pixelSize: 15
                                        font.bold: true
                                        color: text
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: sensor
                                        font.pixelSize: 12
                                        color: subtext
                                    }
                                }

                                Rectangle {
                                    height: 24
                                    width: badgeLabel.width + 16
                                    radius: 6
                                    color: status === "critical" ? "#3a1a1a"
                                         : status === "warning" ? "#3a3a1a"
                                         : "#0a2e1a"
                                    border.color: status === "critical" ? red
                                                : status === "warning" ? yellow
                                                : green
                                    border.width: 1
                                    Label {
                                        id: badgeLabel
                                        anchors.centerIn: parent
                                        text: status.toUpperCase()
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: status === "critical" ? red
                                             : status === "warning" ? yellow
                                             : green
                                    }
                                }
                            }

                            Item { height: 14 }

                            RowLayout {
                                spacing: 4
                                Label {
                                    text: value.toFixed(1)
                                    font.pixelSize: 34
                                    font.bold: true
                                    color: status === "critical" ? red
                                         : status === "warning" ? yellow
                                         : blue
                                }
                                Label {
                                    text: unit
                                    font.pixelSize: 14
                                    color: subtext
                                    anchors.baseline: parent.children[0].baseline
                                    anchors.baselineOffset: 4
                                }
                                Item { Layout.fillWidth: true }
                            }

                            Item { height: 8 }

                            RowLayout {
                                spacing: 6
                                Label {
                                    text: "\u23F1"
                                    font.pixelSize: 11
                                    color: muted
                                }
                                Label {
                                    text: timestamp
                                    font.pixelSize: 11
                                    color: muted
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Label {
        anchors.centerIn: parent
        visible: deviceModel.rowCount === 0
        text: "\u26A0 No devices connected"
        color: muted
        font.pixelSize: 16
    }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        visible: deviceModel.rowCount > 0 && searchText.length > 0
        text: "No devices match your search"
        color: muted
        font.pixelSize: 13
    }
}
