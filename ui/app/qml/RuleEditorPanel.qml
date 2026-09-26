import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Theme.js" as Theme

Rectangle {
    id: root
    color: Theme.bgPanel

    // -1 means the form is in "create" mode; any other value is the id of the
    // rule currently being edited. Save routes to createRule/updateRule on this.
    property int editingId: -1
    property string statusMsg: ""
    property bool statusError: false

    readonly property var conditionSymbols: ({
        "gt": ">", "lt": "<", "gte": "≥", "lte": "≤", "eq": "="
    })

    function conditionLabel(cond) {
        var sym = conditionSymbols[cond]
        return sym ? sym : cond
    }

    // Load (or reload) the rule list. Clear first so a refetch replaces rather
    // than appends — the model is a read-through cache of GET /api/rules.
    function loadRules() {
        ruleModel.clear()
        apiClient.fetchRules()
    }

    function resetForm() {
        editingId = -1
        deviceCombo.currentIndex = 0
        sensorCombo.currentIndex = 0
        conditionCombo.currentIndex = 0
        severityCombo.currentIndex = 1 // warning
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
        var ci = conditionCombo.indexOfValue(r.condition)
        conditionCombo.currentIndex = ci >= 0 ? ci : 0
        var sevi = severityCombo.indexOfValue(r.severity)
        severityCombo.currentIndex = sevi >= 0 ? sevi : 1
        thresholdField.text = String(r.threshold)
        statusMsg = ""
        statusError = false
    }

    function submit() {
        var device = deviceCombo.currentText
        var sensor = sensorCombo.currentText
        var condition = conditionCombo.currentValue
        var severity = severityCombo.currentValue
        var threshold = parseFloat(thresholdField.text)

        if (device.length === 0 || sensor.length === 0 || isNaN(threshold)) {
            statusMsg = "Pick a device and sensor, and enter a numeric threshold."
            statusError = true
            return
        }

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
            if (!ok) {
                root.statusMsg = "Could not load rules: " + error
                root.statusError = true
            }
        }
        function onRuleMutationFinished(ok, error) {
            if (ok) {
                root.statusMsg = (root.editingId < 0 ? "Rule created." : "Rule updated.")
                root.statusError = false
                root.resetForm()
                root.loadRules()
            } else {
                root.statusMsg = "Save failed: " + error
                root.statusError = true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        // —— Editor form ——
        ShadowCard {
            Layout.fillWidth: true
            Layout.preferredHeight: formCol.implicitHeight + Theme.spaceLg * 2
            shadowBlur: 14
            shadowOffsetY: 4

            ColumnLayout {
                id: formCol
                anchors.fill: parent
                anchors.margins: Theme.spaceLg
                spacing: Theme.spaceMd

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm
                    Icon {
                        name: root.editingId < 0 ? "gauge" : "activity"
                        width: 15; height: 15
                        color: Theme.accent
                    }
                    Label {
                        text: root.editingId < 0 ? "New alarm rule"
                                                  : "Editing rule #" + root.editingId
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontMd
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        visible: root.statusMsg.length > 0
                        text: root.statusMsg
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXs
                        color: root.statusError ? Theme.critical : Theme.success
                    }
                }

                // Field row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceMd

                    ColumnLayout {
                        spacing: 3
                        Label {
                            text: "DEVICE"
                            font.family: Theme.fontFamily; font.pixelSize: 10
                            font.bold: true; font.letterSpacing: 0.9; color: Theme.textMuted
                        }
                        ComboBox {
                            id: deviceCombo
                            model: deviceModel
                            textRole: "deviceId"
                            editable: true
                            Layout.preferredWidth: 200
                            contentItem: TextField {
                                text: deviceCombo.editText
                                onTextEdited: deviceCombo.editText = text
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSm
                                leftPadding: 8
                                verticalAlignment: Text.AlignVCenter
                                background: null
                            }
                            background: Rectangle {
                                color: Theme.bgInput; radius: Theme.radiusMd
                                border.color: Theme.border; border.width: 1
                            }
                            indicator: Icon {
                                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: 10; width: 10; height: 10
                                name: "chevron-down"; color: Theme.textMuted
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 3
                        Label {
                            text: "SENSOR"
                            font.family: Theme.fontFamily; font.pixelSize: 10
                            font.bold: true; font.letterSpacing: 0.9; color: Theme.textMuted
                        }
                        ComboBox {
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
                            contentItem: TextField {
                                text: sensorCombo.editText
                                onTextEdited: sensorCombo.editText = text
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSm
                                leftPadding: 8
                                verticalAlignment: Text.AlignVCenter
                                background: null
                            }
                            background: Rectangle {
                                color: Theme.bgInput; radius: Theme.radiusMd
                                border.color: Theme.border; border.width: 1
                            }
                            indicator: Icon {
                                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: 10; width: 10; height: 10
                                name: "chevron-down"; color: Theme.textMuted
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 3
                        Label {
                            text: "CONDITION"
                            font.family: Theme.fontFamily; font.pixelSize: 10
                            font.bold: true; font.letterSpacing: 0.9; color: Theme.textMuted
                        }
                        ComboBox {
                            id: conditionCombo
                            Layout.preferredWidth: 150
                            textRole: "label"
                            valueRole: "value"
                            model: ListModel {
                                ListElement { value: "gt";  label: "> greater than" }
                                ListElement { value: "lt";  label: "< less than" }
                                ListElement { value: "gte"; label: "≥ greater or equal" }
                                ListElement { value: "lte"; label: "≤ less or equal" }
                                ListElement { value: "eq";  label: "= equal to" }
                            }
                            contentItem: Label {
                                text: conditionCombo.currentText
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily; font.pixelSize: Theme.fontSm
                                leftPadding: 8; verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: Theme.bgInput; radius: Theme.radiusMd
                                border.color: Theme.border; border.width: 1
                            }
                            indicator: Icon {
                                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: 10; width: 10; height: 10
                                name: "chevron-down"; color: Theme.textMuted
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 3
                        Label {
                            text: "THRESHOLD"
                            font.family: Theme.fontFamily; font.pixelSize: 10
                            font.bold: true; font.letterSpacing: 0.9; color: Theme.textMuted
                        }
                        TextField {
                            id: thresholdField
                            Layout.preferredWidth: 110
                            placeholderText: "0.0"
                            validator: DoubleValidator {}
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            color: Theme.textPrimary
                            font.family: Theme.fontFamilyMono
                            font.pixelSize: Theme.fontSm
                            leftPadding: 8
                            background: Rectangle {
                                color: Theme.bgInput; radius: Theme.radiusMd
                                border.color: thresholdField.activeFocus ? Theme.accent : Theme.border
                                border.width: 1
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 3
                        Label {
                            text: "SEVERITY"
                            font.family: Theme.fontFamily; font.pixelSize: 10
                            font.bold: true; font.letterSpacing: 0.9; color: Theme.textMuted
                        }
                        ComboBox {
                            id: severityCombo
                            Layout.preferredWidth: 130
                            textRole: "label"
                            valueRole: "value"
                            currentIndex: 1
                            model: ListModel {
                                ListElement { value: "info";     label: "Info" }
                                ListElement { value: "warning";  label: "Warning" }
                                ListElement { value: "critical"; label: "Critical" }
                            }
                            contentItem: Label {
                                text: severityCombo.currentText
                                color: Theme.statusColor(severityCombo.currentValue)
                                font.family: Theme.fontFamily; font.pixelSize: Theme.fontSm
                                font.bold: true
                                leftPadding: 8; verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: Theme.bgInput; radius: Theme.radiusMd
                                border.color: Theme.border; border.width: 1
                            }
                            indicator: Icon {
                                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: 10; width: 10; height: 10
                                name: "chevron-down"; color: Theme.textMuted
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // Action row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    Item { Layout.fillWidth: true }

                    // Cancel — only meaningful while editing
                    Rectangle {
                        visible: root.editingId >= 0
                        height: 34
                        width: cancelRow.implicitWidth + 24
                        radius: Theme.radiusMd
                        color: cancelMouse.containsMouse ? Theme.bgCardHover : "transparent"
                        border.color: Theme.border; border.width: 1
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                        RowLayout {
                            id: cancelRow
                            anchors.centerIn: parent
                            spacing: 6
                            Icon { name: "close"; width: 12; height: 12; color: Theme.textSecondary }
                            Label {
                                text: "Cancel"; font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSm; color: Theme.textSecondary
                            }
                        }
                        MouseArea {
                            id: cancelMouse
                            anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.resetForm()
                        }
                    }

                    // Save / Create
                    Rectangle {
                        height: 34
                        width: saveRow.implicitWidth + 28
                        radius: Theme.radiusMd
                        color: saveMouse.containsMouse ? Theme.accent : Theme.accentSoft
                        border.color: Theme.accent; border.width: 1
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                        RowLayout {
                            id: saveRow
                            anchors.centerIn: parent
                            spacing: 6
                            Icon {
                                name: "check"; width: 12; height: 12
                                color: saveMouse.containsMouse ? Theme.textInverse : Theme.accent
                            }
                            Label {
                                text: root.editingId < 0 ? "Create rule" : "Save changes"
                                font.family: Theme.fontFamily; font.pixelSize: Theme.fontSm
                                font.bold: true
                                color: saveMouse.containsMouse ? Theme.textInverse : Theme.accent
                            }
                        }
                        MouseArea {
                            id: saveMouse
                            anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.submit()
                        }
                    }
                }
            }
        }

        // —— List header ——
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm
            Label {
                text: ruleModel.count + (ruleModel.count === 1 ? " rule" : " rules")
                font.family: Theme.fontFamily; font.pixelSize: Theme.fontSm
                font.bold: true; color: Theme.textSecondary
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                height: 30
                width: refreshRow.implicitWidth + 20
                radius: Theme.radiusMd
                color: refreshMouse.containsMouse ? Theme.bgCardHover : "transparent"
                border.color: Theme.border; border.width: 1
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                RowLayout {
                    id: refreshRow
                    anchors.centerIn: parent
                    spacing: 6
                    Icon { name: "activity"; width: 12; height: 12; color: Theme.textSecondary }
                    Label {
                        text: "Refresh"; font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXs; color: Theme.textSecondary
                    }
                }
                MouseArea {
                    id: refreshMouse
                    anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.loadRules()
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
                    text: "DEVICE / SENSOR"; font.family: Theme.fontFamily; font.pixelSize: 10
                    font.bold: true; font.letterSpacing: 0.9; color: Theme.textMuted
                    Layout.preferredWidth: 240
                }
                Label {
                    text: "CONDITION"; font.family: Theme.fontFamily; font.pixelSize: 10
                    font.bold: true; font.letterSpacing: 0.9; color: Theme.textMuted
                    Layout.fillWidth: true
                }
                Label {
                    text: "THRESHOLD"; font.family: Theme.fontFamily; font.pixelSize: 10
                    font.bold: true; font.letterSpacing: 0.9; color: Theme.textMuted
                    Layout.preferredWidth: 110
                    horizontalAlignment: Text.AlignRight
                }
                Label {
                    text: "SEVERITY"; font.family: Theme.fontFamily; font.pixelSize: 10
                    font.bold: true; font.letterSpacing: 0.9; color: Theme.textMuted
                    Layout.preferredWidth: 100
                }
                Item { Layout.preferredWidth: 160 }
            }
        }

        // Rules list
        ListView {
            id: ruleList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 0
            model: ruleModel
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                id: row
                width: ruleList.width
                height: 54
                color: rowMouse.containsMouse ? Theme.bgCardHover
                       : (index % 2 === 0 ? Theme.bgCard : Theme.bgPanel)
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton
                }
                Rectangle {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.bottom: parent.bottom; height: 1; color: Theme.divider
                }
                Rectangle {
                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: 3; color: Theme.statusColor(severity)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceLg
                    anchors.rightMargin: Theme.spaceLg
                    spacing: Theme.spaceMd

                    ColumnLayout {
                        Layout.preferredWidth: 240
                        spacing: 1
                        Label {
                            text: deviceId
                            font.family: Theme.fontFamily; font.pixelSize: Theme.fontSm
                            font.bold: true; color: Theme.textPrimary
                            elide: Text.ElideRight; Layout.fillWidth: true
                        }
                        Label {
                            text: sensor
                            font.family: Theme.fontFamily; font.pixelSize: Theme.fontXs
                            color: Theme.textMuted; elide: Text.ElideRight; Layout.fillWidth: true
                        }
                    }

                    Label {
                        text: "value " + root.conditionLabel(condition) + " threshold"
                        font.family: Theme.fontFamily; font.pixelSize: Theme.fontSm
                        color: Theme.textSecondary; Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Label {
                        text: threshold.toFixed(2)
                        font.family: Theme.fontFamilyMono; font.pixelSize: Theme.fontSm
                        color: Theme.textSecondary
                        Layout.preferredWidth: 110
                        horizontalAlignment: Text.AlignRight
                    }

                    Rectangle {
                        Layout.preferredWidth: 100
                        height: 24
                        radius: Theme.radiusSm
                        color: Theme.statusBg(severity)
                        border.color: Theme.statusColor(severity); border.width: 1
                        Label {
                            anchors.centerIn: parent
                            text: severity.toUpperCase()
                            font.family: Theme.fontFamily; font.pixelSize: 10
                            font.bold: true; color: Theme.statusColor(severity)
                        }
                    }

                    RowLayout {
                        Layout.preferredWidth: 160
                        spacing: 6

                        Rectangle {
                            height: 30
                            width: editRow.implicitWidth + 18
                            radius: Theme.radiusMd
                            color: editMouse.containsMouse ? Theme.bgElevated : "transparent"
                            border.color: Theme.border; border.width: 1
                            Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                            RowLayout {
                                id: editRow
                                anchors.centerIn: parent; spacing: 5
                                Icon { name: "activity"; width: 11; height: 11; color: Theme.textSecondary }
                                Label {
                                    text: "Edit"; font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontXs; color: Theme.textSecondary
                                }
                            }
                            MouseArea {
                                id: editMouse
                                anchors.fill: parent; hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.editRule(index)
                            }
                        }

                        Rectangle {
                            height: 30
                            width: delRow.implicitWidth + 18
                            radius: Theme.radiusMd
                            color: delMouse.containsMouse ? Theme.criticalBg : "transparent"
                            border.color: delMouse.containsMouse ? Theme.critical : Theme.border
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                            RowLayout {
                                id: delRow
                                anchors.centerIn: parent; spacing: 5
                                Icon {
                                    name: "trash"; width: 11; height: 11
                                    color: delMouse.containsMouse ? Theme.critical : Theme.textSecondary
                                }
                                Label {
                                    text: "Delete"; font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontXs
                                    color: delMouse.containsMouse ? Theme.critical : Theme.textSecondary
                                }
                            }
                            MouseArea {
                                id: delMouse
                                anchors.fill: parent; hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: apiClient.deleteRule(ruleId)
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
        spacing: Theme.spaceMd
        visible: ruleModel.count === 0
        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: "gauge"; width: 32; height: 32; color: Theme.textMuted
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "No alarm rules yet"
            font.family: Theme.fontFamily; font.pixelSize: Theme.fontLg
            font.bold: true; color: Theme.textSecondary
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Create one above to start monitoring a device/sensor threshold."
            font.family: Theme.fontFamily; font.pixelSize: Theme.fontSm; color: Theme.textMuted
        }
    }
}
