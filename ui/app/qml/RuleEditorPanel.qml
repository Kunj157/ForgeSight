import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "transparent"

    // -1 means the form is in "create" mode; any other value is the id of the
    // rule currently being edited. Save routes to createRule/updateRule on this.
    property int editingId: -1
    property string statusMsg: ""
    property bool statusError: false
    property bool saving: false
    property bool loadingRules: false
    // What the in-flight mutation was, so its result is reported correctly
    // (a delete used to be announced as "Rule created.").
    property string lastAction: "create"

    property string conditionValue: "gt"
    property string severityValue: "warning"

    readonly property var conditionSymbols: ({
        "gt": ">", "lt": "<", "gte": "≥", "lte": "≤", "eq": "="
    })
    readonly property var conditionWords: ({
        "gt": "rises above", "lt": "drops below", "gte": "reaches or exceeds",
        "lte": "falls to or below", "eq": "equals"
    })

    function conditionLabel(cond) {
        var sym = conditionSymbols[cond]
        return sym ? sym : cond
    }

    // Load (or reload) the rule list. Clear first so a refetch replaces rather
    // than appends — the model is a read-through cache of GET /api/rules.
    function loadRules() {
        loadingRules = true
        ruleModel.clear()
        apiClient.fetchRules()
    }

    function resetForm() {
        editingId = -1
        deviceCombo.currentIndex = 0
        sensorCombo.currentIndex = 0
        conditionValue = "gt"
        severityValue = "warning"
        thresholdField.text = ""
        statusMsg = ""
        statusError = false
    }

    function editRule(row) {
        var r = ruleModel.get(row)
        if (!r || r.ruleId === undefined)
            return
        editingId = r.ruleId
        var di = deviceCombo.find(r.deviceId)
        deviceCombo.currentIndex = di >= 0 ? di : 0
        var si = sensorCombo.find(r.sensor)
        sensorCombo.currentIndex = si >= 0 ? si : 0
        conditionValue = conditionSymbols[r.condition] ? r.condition : "gt"
        severityValue = ["info", "warning", "critical"].indexOf(r.severity) >= 0 ? r.severity : "warning"
        thresholdField.text = String(r.threshold)
        statusMsg = ""
        statusError = false
    }

    function submit() {
        var device = deviceCombo.currentText
        var sensor = sensorCombo.currentText
        var condition = conditionValue
        var severity = severityValue
        var threshold = parseFloat(thresholdField.text)

        if (device.length === 0 || sensor.length === 0 || isNaN(threshold)) {
            statusMsg = "Pick a device and sensor, and enter a numeric threshold."
            statusError = true
            if (!Theme.reducedMotion)
                shake.restart()
            return
        }

        saving = true
        lastAction = editingId < 0 ? "create" : "update"
        if (editingId < 0)
            apiClient.createRule(device, sensor, condition, threshold, severity)
        else
            apiClient.updateRule(editingId, device, sensor, condition, threshold, severity)
    }

    Component.onCompleted: loadRules()

    Connections {
        target: apiClient
        function onRuleReceived(id, deviceId, sensor, condition, threshold, severity) {
            ruleModel.add_rule(id, deviceId, sensor, condition, threshold, severity)
        }
        function onRulesLoadFinished(ok, error) {
            root.loadingRules = false
            if (!ok) {
                root.statusMsg = "Could not load rules: " + error
                root.statusError = true
            }
        }
        function onRuleMutationFinished(ok, error) {
            root.saving = false
            if (ok) {
                var msg = root.lastAction === "delete" ? "Rule deleted."
                        : root.lastAction === "update" ? "Rule updated." : "Rule created."
                Window.window.toast(msg, root.lastAction === "delete" ? ""
                                    : deviceCombo.currentText + " · " + sensorCombo.currentText, "ok")
                root.resetForm()
                root.statusMsg = msg
                root.statusError = false
                root.loadRules()
            } else {
                root.statusMsg = "Save failed: " + error
                root.statusError = true
                Window.window.toast("Save failed", error, "critical")
            }
        }
    }

    component FieldLabel: Text {
        font.family: Theme.fontFamily
        font.pixelSize: 10
        font.bold: true
        font.letterSpacing: 0.9
        color: Theme.textMuted
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        // —— Editor form ——
        GlowCard {
            id: formCard
            Layout.fillWidth: true
            Layout.preferredHeight: formCol.implicitHeight + Theme.spaceLg * 2
            interactive: false
            elevation: 2
            glow: root.editingId >= 0
            glowColor: Theme.accent
            borderColor: root.editingId >= 0 ? Theme.alpha(Theme.accent, 0.45) : Theme.border

            // Invalid-submit shake.
            transform: Translate { id: shakeShift }
            SequentialAnimation {
                id: shake
                NumberAnimation { target: shakeShift; property: "x"; to: -8; duration: 50 }
                NumberAnimation { target: shakeShift; property: "x"; to: 7; duration: 60 }
                NumberAnimation { target: shakeShift; property: "x"; to: -4; duration: 60 }
                NumberAnimation { target: shakeShift; property: "x"; to: 0; duration: 70 }
            }

            ColumnLayout {
                id: formCol
                anchors.fill: parent
                anchors.margins: Theme.spaceLg
                spacing: Theme.spaceMd

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm
                    Rectangle {
                        implicitWidth: 28; implicitHeight: 28
                        radius: Theme.radiusSm
                        color: Theme.accentSoft
                        Icon {
                            anchors.centerIn: parent
                            name: root.editingId < 0 ? "plus" : "edit"
                            width: 13; height: 13
                            color: Theme.accent
                        }
                    }
                    Text {
                        text: root.editingId < 0 ? "New alarm rule" : "Editing rule #" + root.editingId
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontLg
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    Item { Layout.fillWidth: true }
                    StatusPill {
                        visible: root.statusMsg.length > 0
                        status: root.statusError ? "critical" : "ok"
                        text: root.statusMsg.replace(/\.$/, "")
                        Layout.maximumWidth: 460
                    }
                }

                // Field row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceMd

                    ColumnLayout {
                        spacing: 4
                        FieldLabel { text: "DEVICE" }
                        FxCombo {
                            id: deviceCombo
                            model: deviceModel
                            textRole: "deviceId"
                            editable: true
                            Layout.preferredWidth: 190
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        FieldLabel { text: "SENSOR" }
                        FxCombo {
                            id: sensorCombo
                            editable: true
                            model: ListModel {
                                ListElement { text: "temperature" }
                                ListElement { text: "pressure" }
                                ListElement { text: "vibration" }
                                ListElement { text: "flow" }
                                ListElement { text: "current" }
                            }
                            textRole: "text"
                            Layout.preferredWidth: 150
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        FieldLabel { text: "CONDITION" }
                        SegmentedControl {
                            implicitHeight: 34
                            currentValue: root.conditionValue
                            onActivated: function(v) { root.conditionValue = v }
                            model: [
                                { label: ">", value: "gt" },
                                { label: "<", value: "lt" },
                                { label: "≥", value: "gte" },
                                { label: "≤", value: "lte" },
                                { label: "=", value: "eq" },
                            ]
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        FieldLabel { text: "THRESHOLD" }
                        TextField {
                            id: thresholdField
                            Layout.preferredWidth: 120
                            implicitHeight: 34
                            placeholderText: "0.0"
                            placeholderTextColor: Theme.textMuted
                            validator: DoubleValidator {}
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            color: Theme.textPrimary
                            font.family: Theme.fontFamilyMono
                            font.pixelSize: Theme.fontMd
                            leftPadding: 12
                            selectByMouse: true
                            onAccepted: root.submit()
                            background: Rectangle {
                                color: Theme.bgInput
                                radius: Theme.radiusMd
                                border.width: 1
                                border.color: root.statusError && thresholdField.text.length === 0 ? Theme.critical
                                            : thresholdField.activeFocus ? Theme.accent : Theme.border
                                Behavior on border.color { ColorAnimation { duration: Theme.motionFast } }
                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: -3
                                    radius: parent.radius + 3
                                    color: "transparent"
                                    border.width: 3
                                    border.color: Theme.alpha(Theme.accent, 0.18)
                                    opacity: thresholdField.activeFocus ? 1 : 0
                                    Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        FieldLabel { text: "SEVERITY" }
                        SegmentedControl {
                            implicitHeight: 34
                            currentValue: root.severityValue
                            onActivated: function(v) { root.severityValue = v }
                            model: [
                                { label: "Info", value: "info", tone: "info" },
                                { label: "Warning", value: "warning", tone: "warning" },
                                { label: "Critical", value: "critical", tone: "critical" },
                            ]
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // Live sentence preview + actions
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 34
                        radius: Theme.radiusMd
                        color: Theme.alpha(Theme.statusColor(root.severityValue), 0.07)
                        border.width: 1
                        border.color: Theme.alpha(Theme.statusColor(root.severityValue), 0.25)
                        Behavior on color { ColorAnimation { duration: Theme.motionMed } }

                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spaceMd
                            anchors.rightMargin: Theme.spaceMd
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            textFormat: Text.StyledText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSm
                            color: Theme.textSecondary
                            text: "Raise a <b><font color='" + Theme.statusColor(root.severityValue) + "'>"
                                  + root.severityValue.toUpperCase() + "</font></b> alarm when <b>"
                                  + (deviceCombo.editText || deviceCombo.currentText || "…") + "</b> "
                                  + (sensorCombo.editText || sensorCombo.currentText || "…") + " "
                                  + root.conditionWords[root.conditionValue] + " <b>"
                                  + (thresholdField.text.length > 0 ? thresholdField.text : "…") + "</b>"
                        }
                    }

                    FxButton {
                        visible: root.editingId >= 0
                        text: "Cancel"
                        icon: "close"
                        onClicked: root.resetForm()
                    }
                    FxButton {
                        text: root.editingId < 0 ? "Create rule" : "Save changes"
                        icon: "check"
                        variant: "primary"
                        busy: root.saving
                        implicitWidth: 140
                        onClicked: root.submit()
                    }
                }
            }
        }

        // —— List header ——
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm
            Text {
                text: ruleModel.count + (ruleModel.count === 1 ? " rule" : " rules")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontMd
                font.bold: true
                color: Theme.textSecondary
            }
            Text {
                text: "· changes apply on the alarm engine's next poll"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontXs
                color: Theme.textMuted
            }
            Item { Layout.fillWidth: true }
            FxButton {
                text: "Refresh"
                icon: "refresh"
                compact: true
                busy: root.loadingRules
                onClicked: root.loadRules()
            }
        }

        // —— Rules table ——
        GlowCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            interactive: false
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    color: Theme.alpha(Theme.bgElevated, 0.6)
                    radius: Theme.radiusLg - 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spaceLg + 4
                        anchors.rightMargin: Theme.spaceLg
                        spacing: Theme.spaceMd
                        FieldLabel { text: "DEVICE / SENSOR"; Layout.preferredWidth: 220 }
                        FieldLabel { text: "CONDITION"; Layout.fillWidth: true }
                        FieldLabel { text: "SEVERITY"; Layout.preferredWidth: 110 }
                        Item { Layout.preferredWidth: 170 }
                    }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.divider }
                }

                ListView {
                    id: ruleList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: ruleModel
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    add: Transition {
                        enabled: !Theme.reducedMotion
                        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motionSlow }
                    }

                    delegate: Item {
                        id: row
                        width: ruleList.width
                        height: 56
                        readonly property bool editing: ruleId === root.editingId

                        Rectangle {
                            anchors.fill: parent
                            color: row.editing ? Theme.accentSoft
                                 : rowHover.hovered ? Theme.bgCardHover : "transparent"
                            Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                        }
                        HoverHandler { id: rowHover }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.divider }
                        Rectangle {
                            x: 0; width: 3; radius: 1.5
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: 8
                            color: row.editing ? Theme.accent : Theme.statusColor(severity)
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spaceLg + 4
                            anchors.rightMargin: Theme.spaceLg
                            spacing: Theme.spaceMd

                            ColumnLayout {
                                Layout.preferredWidth: 220
                                Layout.fillWidth: false
                                spacing: 1
                                Text {
                                    text: deviceId
                                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontSm
                                    font.bold: true; color: Theme.textPrimary
                                    elide: Text.ElideRight; Layout.fillWidth: true
                                }
                                Text {
                                    text: sensor
                                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontXs
                                    color: Theme.textMuted; elide: Text.ElideRight; Layout.fillWidth: true
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                implicitHeight: 26
                                Rectangle {
                                    anchors.verticalCenter: parent.verticalCenter
                                    implicitWidth: condRow.implicitWidth + 20
                                    implicitHeight: 26
                                    radius: 13
                                    color: Theme.bgElevated
                                    border.width: 1
                                    border.color: Theme.border
                                    Row {
                                        id: condRow
                                        anchors.centerIn: parent
                                        spacing: 6
                                        Text { text: sensor; font.family: Theme.fontFamily; font.pixelSize: Theme.fontXs; color: Theme.textSecondary }
                                        Text { text: root.conditionLabel(condition); font.family: Theme.fontFamilyMono; font.pixelSize: Theme.fontSm; font.bold: true; color: Theme.accent }
                                        Text { text: threshold.toFixed(2); font.family: Theme.fontFamilyMono; font.pixelSize: Theme.fontXs; font.bold: true; color: Theme.textPrimary }
                                    }
                                }
                            }

                            Item {
                                Layout.preferredWidth: 110
                                implicitHeight: 22
                                StatusPill { status: severity; anchors.verticalCenter: parent.verticalCenter }
                            }

                            Row {
                                Layout.preferredWidth: 170
                                layoutDirection: Qt.RightToLeft
                                spacing: 6
                                FxButton {
                                    text: "Delete"
                                    icon: "trash"
                                    variant: "danger"
                                    compact: true
                                    onClicked: {
                                        root.lastAction = "delete"
                                        apiClient.deleteRule(ruleId)
                                    }
                                }
                                FxButton {
                                    text: row.editing ? "Editing" : "Edit"
                                    icon: "edit"
                                    compact: true
                                    enabled: !row.editing
                                    onClicked: root.editRule(index)
                                }
                            }
                        }
                    }
                }
            }

            // Loading skeleton rows
            Column {
                anchors.fill: parent
                anchors.topMargin: 52
                anchors.leftMargin: Theme.spaceLg
                anchors.rightMargin: Theme.spaceLg
                spacing: 22
                visible: root.loadingRules && ruleModel.count === 0
                Repeater {
                    model: 4
                    Row {
                        spacing: Theme.spaceLg
                        Skeleton { width: 200; height: 14 }
                        Skeleton { width: 260; height: 14 }
                        Skeleton { width: 80; height: 14; radius: 7 }
                    }
                }
            }

            EmptyState {
                anchors.centerIn: parent
                visible: ruleModel.count === 0 && !root.loadingRules
                icon: "gauge"
                title: "No alarm rules yet"
                subtitle: "Create one above to start monitoring a device/sensor threshold."
            }
        }
    }
}
