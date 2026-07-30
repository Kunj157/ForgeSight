import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Theme.js" as Theme

Rectangle {
    color: Theme.bgPanel

    property string searchText: ""

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
                font.pixelSize: Theme.fontSm
                color: Theme.textSecondary
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                height: 34
                width: 260
                radius: Theme.radiusMd
                color: Theme.bgInput
                border.color: searchInput.activeFocus ? Theme.accent : Theme.border
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceMd
                    anchors.rightMargin: Theme.spaceMd
                    spacing: Theme.spaceSm

                    Label {
                        text: "Filter"
                        font.pixelSize: Theme.fontXs
                        color: Theme.textMuted
                    }

                    TextInput {
                        id: searchInput
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSm
                        Layout.fillWidth: true
                        clip: true
                        selectByMouse: true
                        onTextChanged: searchText = text
                    }

                    Label {
                        text: "Clear"
                        font.pixelSize: Theme.fontXs
                        color: Theme.accent
                        visible: searchInput.text.length > 0
                        MouseArea {
                            anchors.fill: parent
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

        // Device grid
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            Flow {
                width: parent.width
                spacing: Theme.spaceMd

                Repeater {
                    model: deviceModel

                    delegate: Rectangle {
                        id: card
                        width: Math.min(320, Math.max(260, (parent.width - Theme.spaceMd) / 3 - 1))
                        height: 148
                        radius: Theme.radiusLg
                        color: Theme.bgCard
                        border.color: mouseArea.containsMouse ? Theme.borderStrong : Theme.border
                        border.width: 1

                        opacity: {
                            if (searchText.length === 0) return 1.0
                            var q = searchText.toLowerCase()
                            return deviceId.toLowerCase().indexOf(q) >= 0
                                   || sensor.toLowerCase().indexOf(q) >= 0
                                   ? 1.0 : 0.28
                        }
                        visible: opacity > 0.2 || searchText.length === 0

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
                            radius: 1
                            color: Theme.statusColor(status)
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spaceLg + 4
                            anchors.rightMargin: Theme.spaceLg
                            anchors.topMargin: Theme.spaceMd
                            anchors.bottomMargin: Theme.spaceMd
                            spacing: 0

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spaceSm

                                ColumnLayout {
                                    spacing: 2
                                    Layout.fillWidth: true
                                    Label {
                                        text: deviceId
                                        font.pixelSize: Theme.fontMd
                                        font.bold: true
                                        color: Theme.textPrimary
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: sensor
                                        font.pixelSize: Theme.fontSm
                                        color: Theme.textSecondary
                                    }
                                }

                                Rectangle {
                                    height: 22
                                    width: badgeLabel.implicitWidth + 14
                                    radius: Theme.radiusSm
                                    color: Theme.statusBg(status)
                                    border.color: Theme.statusColor(status)
                                    border.width: status === "normal" ? 0 : 1
                                    Label {
                                        id: badgeLabel
                                        anchors.centerIn: parent
                                        text: status.toUpperCase()
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: Theme.statusColor(status)
                                    }
                                }
                            }

                            Item { Layout.fillHeight: true }

                            RowLayout {
                                spacing: 6
                                Label {
                                    text: value.toFixed(1)
                                    font.pixelSize: Theme.fontDisplay
                                    font.bold: true
                                    color: status === "normal"
                                           ? Theme.textPrimary
                                           : Theme.statusColor(status)
                                    Layout.alignment: Qt.AlignBaseline
                                }
                                Label {
                                    text: unit
                                    font.pixelSize: Theme.fontSm
                                    color: Theme.textMuted
                                    Layout.alignment: Qt.AlignBaseline
                                    Layout.bottomMargin: 4
                                }
                                Item { Layout.fillWidth: true }
                            }

                            Label {
                                text: timestamp
                                font.pixelSize: Theme.fontXs
                                color: Theme.textMuted
                                elide: Text.ElideRight
                                Layout.fillWidth: true
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
        spacing: Theme.spaceSm
        visible: deviceModel.count === 0

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "No devices connected"
            font.pixelSize: Theme.fontLg
            font.bold: true
            color: Theme.textSecondary
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Start the simulator and ingestion service to see live readings."
            font.pixelSize: Theme.fontSm
            color: Theme.textMuted
        }
    }
}
