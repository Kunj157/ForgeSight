import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel
    color: "transparent"

    property string filterSeverity: "all"
    // Alarm ids acknowledged while offline and queued for sync — tracked
    // locally so the row can show "Queued" instead of "Acknowledge" until
    // the flush completes and alarmModel reports it as truly acknowledged.
    property var queuedAckIds: ({})
    property int tick: 0

    // Re-evaluated whenever the model's counts move.
    function countOf(sev, unackedOnly) {
        alarmModel.count
        alarmModel.unacknowledgedCount
        return alarmModel.count_matching(sev, unackedOnly)
    }

    Timer { interval: 5000; running: true; repeat: true; onTriggered: panel.tick++ }

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

    component Kpi: GlowCard {
        id: kpi
        property string label: ""
        property int value: 0
        property string tone: "normal"
        property string caption: ""
        Layout.fillWidth: true
        Layout.preferredHeight: 84
        trail: tone === "critical" && value > 0
        borderColor: value > 0 && tone !== "normal" ? Theme.alpha(Theme.statusColor(tone), 0.4) : Theme.border
        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.spaceLg
            spacing: Theme.spaceMd
            ColumnLayout {
                spacing: 2
                Layout.fillWidth: true
                Text {
                    text: kpi.label
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: Theme.trackingCaps
                    color: Theme.textMuted
                }
                SlidingNumber {
                    value: kpi.value
                    font.pixelSize: 26
                    font.bold: true
                    color: kpi.value > 0 && kpi.tone !== "normal" ? Theme.statusColor(kpi.tone) : Theme.textPrimary
                }
            }
            Text {
                text: kpi.caption
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontXs
                color: Theme.textMuted
                Layout.alignment: Qt.AlignBottom
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        // —— Summary ——
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceLg
            Kpi { label: "UNACKNOWLEDGED"; value: alarmModel.unacknowledgedCount; tone: "critical"; caption: "need attention" }
            Kpi { label: "CRITICAL"; value: panel.countOf("critical", true); tone: "critical"; caption: "active" }
            Kpi { label: "WARNING"; value: panel.countOf("warning", true); tone: "warning"; caption: "active" }
            Kpi { label: "ACKNOWLEDGED"; value: alarmModel.count - alarmModel.unacknowledgedCount; tone: "ok"; caption: "of " + alarmModel.count + " total" }
        }

        // —— Toolbar ——
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            StatusPill {
                visible: alarmModel.unacknowledgedCount > 0
                status: "critical"
                pulse: true
                text: alarmModel.unacknowledgedCount + " unacknowledged"
                implicitHeight: 26
            }
            StatusPill {
                visible: alarmModel.count > 0 && alarmModel.unacknowledgedCount === 0
                status: "ok"
                text: "all acknowledged"
                implicitHeight: 26
            }

            Item { Layout.fillWidth: true }

            Icon { name: "filter"; width: 13; height: 13; color: Theme.textMuted }

            SegmentedControl {
                currentValue: panel.filterSeverity
                onActivated: function(value) { panel.filterSeverity = value }
                model: [
                    { label: "All", value: "all" },
                    { label: "Critical", value: "critical", tone: "critical" },
                    { label: "Warning", value: "warning", tone: "warning" },
                    { label: "Info", value: "info", tone: "info" },
                ]
            }

            FxButton {
                text: "Clear all"
                icon: "trash"
                compact: true
                enabled: alarmModel.count > 0
                onClicked: alarmModel.clear()
            }
        }

        // —— Table ——
        GlowCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            interactive: false
            radius: Theme.radiusLg
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                // Column headers
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    color: Theme.alpha(Theme.bgElevated, 0.6)
                    radius: Theme.radiusLg - 1

                    component Head: Text {
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 0.9
                        color: Theme.textMuted
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spaceLg + 4
                        anchors.rightMargin: Theme.spaceLg
                        spacing: Theme.spaceMd
                        Head { text: "SEVERITY"; Layout.preferredWidth: 104 }
                        Head { text: "DEVICE / SENSOR"; Layout.preferredWidth: 170 }
                        Head { text: "MESSAGE"; Layout.fillWidth: true }
                        Head { text: "VALUE"; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                        Head { text: "TIME"; Layout.preferredWidth: 150 }
                        Item { Layout.preferredWidth: 124 }
                    }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.divider }
                }

                ListView {
                    id: alarmList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 0
                    model: alarmModel
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    add: Transition {
                        enabled: !Theme.reducedMotion
                        ParallelAnimation {
                            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motionSlow }
                            NumberAnimation { property: "x"; from: -24; to: 0; duration: Theme.motionSlow; easing.type: Easing.OutCubic }
                        }
                    }
                    displaced: Transition {
                        NumberAnimation { property: "y"; duration: Theme.motionMed; easing.type: Easing.OutCubic }
                    }

                    delegate: Item {
                        id: row
                        width: alarmList.width
                        readonly property bool shown: panel.filterSeverity === "all" || severity === panel.filterSeverity
                        readonly property bool live: !acknowledged
                        readonly property bool queued: model.id in panel.queuedAckIds
                        readonly property color tone: Theme.statusColor(severity)
                        visible: shown
                        height: shown ? 58 : 0

                        // Acknowledge sweep: a success-tinted wash that runs
                        // across the row once when it becomes acknowledged.
                        property real sweep: 0
                        property bool _wasAcked: acknowledged
                        onLiveChanged: {
                            if (!live && !_wasAcked && !Theme.reducedMotion)
                                sweepAnim.restart()
                            _wasAcked = !live
                        }
                        NumberAnimation {
                            id: sweepAnim
                            target: row; property: "sweep"
                            from: 0; to: 1; duration: 700; easing.type: Easing.OutCubic
                        }

                        Rectangle {
                            anchors.fill: parent
                            color: rowHover.hovered ? Theme.bgCardHover
                                 : (row.live && severity === "critical" ? Theme.alpha(Theme.critical, Theme.dark ? 0.05 : 0.035) : "transparent")
                            Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                        }
                        Rectangle {
                            visible: sweepAnim.running
                            height: parent.height
                            width: parent.width * row.sweep
                            opacity: 1 - row.sweep
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: Theme.alpha(Theme.success, 0.0) }
                                GradientStop { position: 1.0; color: Theme.alpha(Theme.success, 0.22) }
                            }
                        }
                        HoverHandler { id: rowHover }

                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.divider }

                        // Severity rail; pulses while a critical alarm is live.
                        Rectangle {
                            id: rail
                            x: 0; width: 3
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: 8
                            radius: 1.5
                            color: row.tone
                            opacity: row.live ? 1 : 0.3
                            SequentialAnimation on opacity {
                                running: row.live && severity === "critical" && row.visible && !Theme.reducedMotion
                                loops: Animation.Infinite
                                NumberAnimation { from: 1; to: 0.35; duration: 700; easing.type: Easing.InOutSine }
                                NumberAnimation { from: 0.35; to: 1; duration: 700; easing.type: Easing.InOutSine }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spaceLg + 4
                            anchors.rightMargin: Theme.spaceLg
                            spacing: Theme.spaceMd
                            opacity: row.live ? 1 : 0.55
                            Behavior on opacity { NumberAnimation { duration: Theme.motionSlow } }

                            Item {
                                Layout.preferredWidth: 104
                                implicitHeight: pill.implicitHeight
                                StatusPill {
                                    id: pill
                                    status: severity
                                    pulse: row.live && severity === "critical"
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 170
                                Layout.fillWidth: false
                                spacing: 1
                                Text {
                                    text: deviceId
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSm
                                    font.bold: true
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: sensor
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontXs
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Text {
                                text: message
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSm
                                color: Theme.textSecondary
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignLeft
                                Layout.fillWidth: true
                                Layout.preferredWidth: 10
                            }

                            Text {
                                text: value.toFixed(1)
                                font.family: Theme.fontFamilyMono
                                font.pixelSize: Theme.fontMd
                                font.bold: true
                                color: row.tone
                                Layout.preferredWidth: 70
                                horizontalAlignment: Text.AlignRight
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 150
                                Layout.fillWidth: false
                                spacing: 1
                                Text {
                                    text: {
                                        var c = timeFormat.clockTime(timestamp)
                                        return c.length > 0 ? c : timestamp
                                    }
                                    font.family: Theme.fontFamilyMono
                                    font.pixelSize: Theme.fontSm
                                    color: Theme.textSecondary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: { panel.tick; return timeFormat.relative(timestamp) }
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                }
                            }

                            Item {
                                Layout.preferredWidth: 124
                                implicitHeight: 30

                                FxButton {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: row.live && !row.queued
                                    text: "Acknowledge"
                                    icon: "check"
                                    variant: "success"
                                    compact: true
                                    onClicked: {
                                        // While offline, queue the ack locally instead of
                                        // firing a request that would just fail — it gets
                                        // flushed automatically once the WS reconnects.
                                        if (wsClient.connected) {
                                            apiClient.acknowledgeAlarm(model.id)
                                        } else {
                                            offlineCache.queueAck(model.id)
                                            var updated = Object.assign({}, panel.queuedAckIds)
                                            updated[model.id] = true
                                            panel.queuedAckIds = updated
                                        }
                                    }
                                }

                                StatusPill {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: row.live && row.queued
                                    status: "warning"
                                    text: "queued"
                                }

                                Row {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: !row.live
                                    spacing: 6
                                    Rectangle {
                                        width: 18; height: 18; radius: 9
                                        color: Theme.successBg
                                        border.width: 1
                                        border.color: Theme.alpha(Theme.success, 0.5)
                                        scale: row.live ? 0 : 1
                                        Behavior on scale { NumberAnimation { duration: Theme.motionSlow; easing.type: Easing.OutBack } }
                                        Icon { anchors.centerIn: parent; name: "check"; width: 9; height: 9; color: Theme.success }
                                    }
                                    Text {
                                        text: "Acked"
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fontXs
                                        color: Theme.success
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    EmptyState {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 60
        visible: alarmModel.count === 0
        icon: "bell"
        tone: "ok"
        title: "No active alarms"
        subtitle: "The plant is operating within configured thresholds."
    }
}
