import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Theme.js" as Theme

Rectangle {
    color: Theme.bgPanel

    property string filterSeverity: "all"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        // Toolbar
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            Label {
                text: alarmModel.unacknowledgedCount + " unacknowledged"
                font.pixelSize: Theme.fontSm
                font.bold: alarmModel.unacknowledgedCount > 0
                color: alarmModel.unacknowledgedCount > 0
                       ? Theme.critical : Theme.textSecondary
            }

            Item { Layout.fillWidth: true }

            Repeater {
                model: [
                    { label: "All", sev: "all" },
                    { label: "Critical", sev: "critical" },
                    { label: "Warning", sev: "warning" },
                    { label: "Info", sev: "info" },
                ]
                delegate: Rectangle {
                    height: 30
                    width: filterLabel.implicitWidth + 18
                    radius: Theme.radiusMd
                    color: filterSeverity === modelData.sev
                           ? Theme.bgElevated : "transparent"
                    border.color: filterSeverity === modelData.sev
                                  ? Theme.borderStrong : Theme.border
                    border.width: 1

                    Label {
                        id: filterLabel
                        anchors.centerIn: parent
                        text: modelData.label
                        font.pixelSize: Theme.fontXs
                        font.bold: filterSeverity === modelData.sev
                        color: {
                            if (filterSeverity !== modelData.sev)
                                return Theme.textSecondary
                            if (modelData.sev === "critical") return Theme.critical
                            if (modelData.sev === "warning") return Theme.warning
                            if (modelData.sev === "info") return Theme.info
                            return Theme.textPrimary
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: filterSeverity = modelData.sev
                    }
                }
            }

            Rectangle {
                height: 30
                width: clearLbl.implicitWidth + 18
                radius: Theme.radiusMd
                color: "transparent"
                border.color: Theme.border
                border.width: 1
                Label {
                    id: clearLbl
                    anchors.centerIn: parent
                    text: "Clear all"
                    font.pixelSize: Theme.fontXs
                    color: Theme.textSecondary
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: alarmModel.clear()
                }
            }
        }

        // Column headers
        Rectangle {
            Layout.fillWidth: true
            height: 32
            color: Theme.bgElevated
            radius: Theme.radiusSm

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceLg
                anchors.rightMargin: Theme.spaceLg
                spacing: Theme.spaceMd

                Label {
                    text: "SEVERITY"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.8
                    color: Theme.textMuted
                    Layout.preferredWidth: 90
                }
                Label {
                    text: "DEVICE / SENSOR"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.8
                    color: Theme.textMuted
                    Layout.preferredWidth: 180
                }
                Label {
                    text: "MESSAGE"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.8
                    color: Theme.textMuted
                    Layout.fillWidth: true
                }
                Label {
                    text: "VALUE"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.8
                    color: Theme.textMuted
                    Layout.preferredWidth: 70
                }
                Label {
                    text: "TIME"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.8
                    color: Theme.textMuted
                    Layout.preferredWidth: 160
                }
                Item { Layout.preferredWidth: 100 }
            }
        }

        // Alarm list — table rows, not cards
        ListView {
            id: alarmList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 0
            model: alarmModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                id: row
                width: alarmList.width
                height: visible ? 52 : 0
                color: index % 2 === 0 ? Theme.bgCard : Theme.bgPanel
                border.color: Theme.divider
                border.width: 0

                // bottom hairline
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.divider
                }

                // left severity rail
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 3
                    color: Theme.statusColor(severity)
                    opacity: acknowledged ? 0.35 : 1.0
                }

                visible: filterSeverity === "all" || severity === filterSeverity
                opacity: acknowledged ? 0.55 : 1.0

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceLg
                    anchors.rightMargin: Theme.spaceLg
                    spacing: Theme.spaceMd

                    Rectangle {
                        Layout.preferredWidth: 90
                        height: 22
                        radius: Theme.radiusSm
                        color: Theme.statusBg(severity)
                        border.color: Theme.statusColor(severity)
                        border.width: 1
                        Label {
                            anchors.centerIn: parent
                            text: severity.toUpperCase()
                            font.pixelSize: 10
                            font.bold: true
                            color: Theme.statusColor(severity)
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 180
                        spacing: 1
                        Label {
                            text: deviceId
                            font.pixelSize: Theme.fontSm
                            font.bold: true
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: sensor
                            font.pixelSize: Theme.fontXs
                            color: Theme.textMuted
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Label {
                        text: message
                        font.pixelSize: Theme.fontSm
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: value.toFixed(1)
                        font.pixelSize: Theme.fontSm
                        font.bold: true
                        color: Theme.statusColor(severity)
                        Layout.preferredWidth: 70
                        horizontalAlignment: Text.AlignRight
                    }

                    Label {
                        text: timestamp
                        font.pixelSize: Theme.fontXs
                        color: Theme.textMuted
                        Layout.preferredWidth: 160
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        Layout.preferredWidth: 100
                        height: 28
                        radius: Theme.radiusMd
                        visible: !acknowledged
                        color: Theme.successBg
                        border.color: Theme.success
                        border.width: 1

                        Label {
                            anchors.centerIn: parent
                            text: "Acknowledge"
                            font.pixelSize: Theme.fontXs
                            font.bold: true
                            color: Theme.success
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var xhr = new XMLHttpRequest()
                                xhr.open("POST",
                                    "http://127.0.0.1:8080/api/alarms/" + model.id + "/ack")
                                xhr.onreadystatechange = function() {
                                    if (xhr.readyState === XMLHttpRequest.DONE
                                            && xhr.status === 200)
                                        alarmModel.acknowledge(model.id)
                                }
                                xhr.send()
                            }
                        }
                    }

                    Label {
                        Layout.preferredWidth: 100
                        visible: acknowledged
                        text: "Acked"
                        font.pixelSize: Theme.fontXs
                        color: Theme.success
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.spaceSm
        visible: alarmModel.count === 0

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "No active alarms"
            font.pixelSize: Theme.fontLg
            font.bold: true
            color: Theme.textSecondary
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "The plant is operating within configured thresholds."
            font.pixelSize: Theme.fontSm
            color: Theme.textMuted
        }
    }
}
