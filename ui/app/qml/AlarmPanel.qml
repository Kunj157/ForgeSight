import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Theme.js" as Theme

Rectangle {
    color: Theme.bgPanel

    property string filterSeverity: "all"
    // Alarm ids acknowledged while offline and queued for sync — tracked
    // locally so the row can show "Queued" instead of "Acknowledge" until
    // the flush completes and alarmModel reports it as truly acknowledged.
    property var queuedAckIds: ({})

    Connections {
        target: alarmModel
        function onAlarmAcknowledged(alarm_id) {
            if (alarm_id in queuedAckIds) {
                var updated = Object.assign({}, queuedAckIds)
                delete updated[alarm_id]
                queuedAckIds = updated
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        // Toolbar
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            RowLayout {
                spacing: 6
                Rectangle {
                    visible: alarmModel.unacknowledgedCount > 0
                    width: 7; height: 7; radius: 3.5
                    color: Theme.critical
                    Layout.alignment: Qt.AlignVCenter
                }
                Label {
                    text: alarmModel.unacknowledgedCount + " unacknowledged"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSm
                    font.bold: alarmModel.unacknowledgedCount > 0
                    color: alarmModel.unacknowledgedCount > 0
                           ? Theme.critical : Theme.textSecondary
                }
            }

            Item { Layout.fillWidth: true }

            Icon {
                name: "filter"
                width: 13; height: 13
                color: Theme.textMuted
                Layout.rightMargin: 2
            }

            Repeater {
                model: [
                    { label: "All", sev: "all" },
                    { label: "Critical", sev: "critical" },
                    { label: "Warning", sev: "warning" },
                    { label: "Info", sev: "info" },
                ]
                delegate: Rectangle {
                    height: 30
                    width: filterLabel.implicitWidth + 20
                    radius: Theme.radiusMd
                    property bool sel: filterSeverity === modelData.sev
                    color: sel ? Theme.bgElevated : (filterMouse.containsMouse ? Theme.bgCardHover : "transparent")
                    border.color: sel ? Theme.borderStrong : Theme.border
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }

                    Label {
                        id: filterLabel
                        anchors.centerIn: parent
                        text: modelData.label
                        font.family: Theme.fontFamily
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
                        id: filterMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: filterSeverity = modelData.sev
                    }
                }
            }

            Rectangle {
                height: 30
                width: clearRow.implicitWidth + 20
                radius: Theme.radiusMd
                color: clearMouse.containsMouse ? Theme.bgCardHover : "transparent"
                border.color: Theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                RowLayout {
                    id: clearRow
                    anchors.centerIn: parent
                    spacing: 6
                    Icon { name: "trash"; width: 12; height: 12; color: Theme.textSecondary }
                    Label {
                        text: "Clear all"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXs
                        color: Theme.textSecondary
                    }
                }
                MouseArea {
                    id: clearMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: alarmModel.clear()
                }
            }
        }

        // Column headers
        Rectangle {
            Layout.fillWidth: true
            height: 34
            color: Theme.bgElevated
            radius: Theme.radiusMd

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceLg
                anchors.rightMargin: Theme.spaceLg
                spacing: Theme.spaceMd

                Label {
                    text: "SEVERITY"
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.9
                    color: Theme.textMuted
                    Layout.preferredWidth: 90
                }
                Label {
                    text: "DEVICE / SENSOR"
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.9
                    color: Theme.textMuted
                    Layout.preferredWidth: 180
                }
                Label {
                    text: "MESSAGE"
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.9
                    color: Theme.textMuted
                    Layout.fillWidth: true
                }
                Label {
                    text: "VALUE"
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.9
                    color: Theme.textMuted
                    Layout.preferredWidth: 70
                }
                Label {
                    text: "TIME"
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 0.9
                    color: Theme.textMuted
                    Layout.preferredWidth: 160
                }
                Item { Layout.preferredWidth: 110 }
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
                height: visible ? 54 : 0
                color: rowMouse.containsMouse ? Theme.bgCardHover
                       : (index % 2 === 0 ? Theme.bgCard : Theme.bgPanel)
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }

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
                        height: 24
                        radius: Theme.radiusSm
                        color: Theme.statusBg(severity)
                        border.color: Theme.statusColor(severity)
                        border.width: 1
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Icon {
                                name: severity === "critical" ? "bell" : "activity"
                                width: 10; height: 10
                                color: Theme.statusColor(severity)
                            }
                            Label {
                                text: severity.toUpperCase()
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                                font.bold: true
                                color: Theme.statusColor(severity)
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 180
                        spacing: 1
                        Label {
                            text: deviceId
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSm
                            font.bold: true
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: sensor
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontXs
                            color: Theme.textMuted
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Label {
                        text: message
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSm
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: value.toFixed(1)
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontSm
                        font.bold: true
                        color: Theme.statusColor(severity)
                        Layout.preferredWidth: 70
                        horizontalAlignment: Text.AlignRight
                    }

                    Label {
                        text: timestamp
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontXs
                        color: Theme.textMuted
                        Layout.preferredWidth: 160
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        id: ackButton
                        Layout.preferredWidth: 110
                        height: 30
                        radius: Theme.radiusMd
                        property bool queued: model.id in queuedAckIds
                        visible: !acknowledged
                        color: queued ? Theme.bgElevated
                               : (ackMouse.containsMouse ? Theme.success : Theme.successBg)
                        border.color: queued ? Theme.border : Theme.success
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 5
                            Icon {
                                name: ackButton.queued ? "clock" : "check"
                                width: 11; height: 11
                                color: ackButton.queued ? Theme.textMuted
                                       : (ackMouse.containsMouse ? Theme.textInverse : Theme.success)
                            }
                            Label {
                                text: ackButton.queued ? "Queued" : "Acknowledge"
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontXs
                                font.bold: true
                                color: ackButton.queued ? Theme.textMuted
                                       : (ackMouse.containsMouse ? Theme.textInverse : Theme.success)
                            }
                        }

                        MouseArea {
                            id: ackMouse
                            anchors.fill: parent
                            hoverEnabled: !ackButton.queued
                            enabled: !ackButton.queued
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                // While offline, queue the ack locally instead of
                                // firing a request that would just fail — it gets
                                // flushed automatically once the WS reconnects.
                                if (wsClient.connected) {
                                    apiClient.acknowledgeAlarm(model.id)
                                } else {
                                    offlineCache.queueAck(model.id)
                                    var updated = Object.assign({}, queuedAckIds)
                                    updated[model.id] = true
                                    queuedAckIds = updated
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.preferredWidth: 110
                        visible: acknowledged
                        spacing: 5
                        Icon { name: "check"; width: 11; height: 11; color: Theme.success }
                        Label {
                            text: "Acked"
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontXs
                            color: Theme.success
                        }
                    }
                }
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.spaceMd
        visible: alarmModel.count === 0

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: "bell"
            width: 32; height: 32
            color: Theme.textMuted
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "No active alarms"
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLg
            font.bold: true
            color: Theme.textSecondary
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "The plant is operating within configured thresholds."
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            color: Theme.textMuted
        }
    }
}
