import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import "Theme.js" as Theme

Rectangle {
    id: root
    color: Theme.bgPanel

    property bool loading: false
    property int selectedRangeHours: 24
    property double minVal: 0
    property double maxVal: 0
    property double minTs: 0
    property double maxTs: 0

    Connections {
        target: apiClient
        function onHistoryPointReceived(deviceId, sensor, value, unit, timestamp, anomaly) {
            historyModel.add_point(deviceId, sensor, value, unit, timestamp, anomaly)
            var ts = Date.parse(timestamp)
            lineSeries.append(ts, value)
            if (anomaly)
                anomalySeries.append(ts, value)

            if (historyModel.point_count() === 1) {
                root.minVal = value; root.maxVal = value
                root.minTs = ts; root.maxTs = ts
            } else {
                if (value < root.minVal) root.minVal = value
                if (value > root.maxVal) root.maxVal = value
                if (ts < root.minTs) root.minTs = ts
                if (ts > root.maxTs) root.maxTs = ts
            }
        }
        function onHistoryLoadFinished(ok, error) {
            loading = false
            if (!ok) {
                statusText.text = "Load failed: " + error
                return
            }
            if (historyModel.point_count() > 0) {
                var margin = (root.maxVal - root.minVal) * 0.1
                if (margin === 0) margin = 1
                axisY.min = root.minVal - margin
                axisY.max = root.maxVal + margin
                axisX.min = new Date(root.minTs)
                axisX.max = new Date(root.maxTs)
            }
            statusText.text = "Loaded " + historyModel.point_count() + " points"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        // Query toolbar
        ShadowCard {
            Layout.fillWidth: true
            height: 56
            shadowBlur: 12
            shadowOffsetY: 3

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceLg
                anchors.rightMargin: Theme.spaceLg
                spacing: Theme.spaceMd

                Label {
                    text: "Device"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontXs
                    color: Theme.textMuted
                }
                ComboBox {
                    id: deviceCombo
                    model: deviceModel
                    textRole: "deviceId"
                    Layout.preferredWidth: 160
                    contentItem: Label {
                        text: parent.currentText
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSm
                        leftPadding: 8
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: Theme.bgInput
                        radius: Theme.radiusMd
                        border.color: Theme.border
                        border.width: 1
                    }
                    indicator: Icon {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 10
                        width: 10; height: 10
                        name: "chevron-down"
                        color: Theme.textMuted
                    }
                }

                Label {
                    text: "Sensor"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontXs
                    color: Theme.textMuted
                }
                ComboBox {
                    id: sensorCombo
                    model: ListModel {
                        ListElement { text: "temperature" }
                        ListElement { text: "pressure" }
                        ListElement { text: "vibration" }
                        ListElement { text: "flow" }
                        ListElement { text: "current" }
                    }
                    textRole: "text"
                    Layout.preferredWidth: 140
                    contentItem: Label {
                        text: parent.currentText
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSm
                        leftPadding: 8
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: Theme.bgInput
                        radius: Theme.radiusMd
                        border.color: Theme.border
                        border.width: 1
                    }
                    indicator: Icon {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 10
                        width: 10; height: 10
                        name: "chevron-down"
                        color: Theme.textMuted
                    }
                }

                Rectangle {
                    width: 1; height: 24
                    color: Theme.divider
                }

                Repeater {
                    model: [
                        { label: "1H", hours: 1 },
                        { label: "6H", hours: 6 },
                        { label: "24H", hours: 24 },
                        { label: "7D", hours: 168 },
                    ]
                    delegate: Rectangle {
                        height: 30
                        width: rangeLbl.implicitWidth + 18
                        radius: Theme.radiusMd
                        property bool sel: selectedRangeHours === modelData.hours
                        color: sel ? Theme.accentSoft
                               : (rangeMouse.containsMouse ? Theme.bgCardHover : "transparent")
                        border.color: sel ? Theme.accent : Theme.border
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                        Label {
                            id: rangeLbl
                            anchors.centerIn: parent
                            text: modelData.label
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontXs
                            font.bold: selectedRangeHours === modelData.hours
                            color: selectedRangeHours === modelData.hours
                                   ? Theme.accent : Theme.textSecondary
                        }
                        MouseArea {
                            id: rangeMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                selectedRangeHours = modelData.hours
                                var d = new Date()
                                d.setHours(d.getHours() - modelData.hours)
                                fromDate.text = d.toISOString()
                                loadHistory()
                            }
                        }
                    }
                }

                Rectangle {
                    height: 30
                    Layout.preferredWidth: 170
                    radius: Theme.radiusMd
                    color: Theme.bgInput
                    border.color: Theme.border
                    border.width: 1
                    TextInput {
                        id: fromDate
                        anchors.fill: parent
                        anchors.margins: 6
                        text: {
                            var d = new Date()
                            d.setHours(d.getHours() - 24)
                            return d.toISOString()
                        }
                        color: Theme.textPrimary
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontXs
                        clip: true
                        selectByMouse: true
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    height: 32
                    width: loadLbl.implicitWidth + 30
                    radius: Theme.radiusMd
                    color: loading ? Theme.bgElevated : (loadMouse.containsMouse ? Theme.accent2 : Theme.accent)
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                    Label {
                        id: loadLbl
                        anchors.centerIn: parent
                        text: loading ? "Loading…" : "Load"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSm
                        font.bold: true
                        color: loading ? Theme.textMuted : "#ffffff"
                    }
                    MouseArea {
                        id: loadMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        enabled: !loading
                        onClicked: loadHistory()
                    }
                }
            }
        }

        // Chart panel
        ShadowCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            shadowBlur: 20
            shadowOffsetY: 6

            ChartView {
                id: chartView
                anchors.fill: parent
                anchors.margins: 4
                antialiasing: true
                backgroundColor: "transparent"
                legend.visible: true
                legend.labelColor: Theme.textSecondary
                legend.font.pixelSize: Theme.fontXs
                legend.alignment: Qt.AlignTop
                legend.backgroundVisible: false
                margins.top: 8
                margins.bottom: 8
                margins.left: 8
                margins.right: 8

                DateTimeAxis {
                    id: axisX
                    format: "MM-dd HH:mm"
                    labelsColor: Theme.textMuted
                    gridLineColor: Theme.divider
                    minorGridVisible: false
                    labelsAngle: -35
                    tickCount: 6
                    color: Theme.border
                }

                ValueAxis {
                    id: axisY
                    labelsColor: Theme.textMuted
                    gridLineColor: Theme.divider
                    labelFormat: "%.1f"
                    color: Theme.border
                }

                LineSeries {
                    id: lineSeries
                    name: "Readings"
                    color: Theme.accent
                    width: 2
                    axisX: axisX
                    axisY: axisY
                }

                ScatterSeries {
                    id: anomalySeries
                    name: "Anomalies"
                    color: Theme.critical
                    markerSize: 7
                    borderColor: "transparent"
                    axisX: axisX
                    axisY: axisY
                }
            }

            Label {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Theme.spaceMd
                text: "Scroll to zoom"
                font.family: Theme.fontFamily
                font.pixelSize: 10
                color: Theme.textMuted
                opacity: 0.7
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                onWheel: function(wheel) {
                    if (wheel.angleDelta.y > 0)
                        chartView.zoomIn()
                    else
                        chartView.zoomOut()
                }
            }

            // Empty chart hint
            ColumnLayout {
                anchors.centerIn: parent
                visible: historyModel.pointCount === 0 && !loading
                spacing: Theme.spaceSm
                Icon {
                    Layout.alignment: Qt.AlignHCenter
                    name: "chart"
                    width: 26; height: 26
                    color: Theme.textMuted
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Select a device and range, then Load"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontMd
                    color: Theme.textMuted
                }
            }
        }

        // Footer actions
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            Rectangle {
                height: 34
                width: csvRow.implicitWidth + 26
                radius: Theme.radiusMd
                color: csvMouse.containsMouse ? Theme.bgCardHover : Theme.bgCard
                border.color: Theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                RowLayout {
                    id: csvRow
                    anchors.centerIn: parent
                    spacing: 6
                    Icon { name: "download"; width: 12; height: 12; color: Theme.textSecondary }
                    Label {
                        text: "Export CSV"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXs
                        font.bold: true
                        color: Theme.textSecondary
                    }
                }
                MouseArea {
                    id: csvMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var path = historyModel.default_export_path("history_export.csv")
                        if (historyModel.export_csv(path))
                            statusText.text = "Exported to " + path
                        else
                            statusText.text = "CSV export failed"
                    }
                }
            }

            Rectangle {
                height: 34
                width: pdfRow.implicitWidth + 26
                radius: Theme.radiusMd
                color: pdfMouse.containsMouse ? Theme.bgCardHover : Theme.bgCard
                border.color: Theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                RowLayout {
                    id: pdfRow
                    anchors.centerIn: parent
                    spacing: 6
                    Icon { name: "download"; width: 12; height: 12; color: Theme.textSecondary }
                    Label {
                        text: "Export PDF"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXs
                        font.bold: true
                        color: Theme.textSecondary
                    }
                }
                MouseArea {
                    id: pdfMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var path = historyModel.default_export_path("history_export.pdf")
                        if (historyModel.export_pdf(path, Window.window))
                            statusText.text = "Exported to " + path
                        else
                            statusText.text = "PDF export failed"
                    }
                }
            }

            Rectangle {
                height: 34
                width: clearRow.implicitWidth + 26
                radius: Theme.radiusMd
                color: clearAllMouse.containsMouse ? Theme.bgCardHover : "transparent"
                border.color: Theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                RowLayout {
                    id: clearRow
                    anchors.centerIn: parent
                    spacing: 6
                    Icon { name: "trash"; width: 12; height: 12; color: Theme.textSecondary }
                    Label {
                        text: "Clear"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXs
                        color: Theme.textSecondary
                    }
                }
                MouseArea {
                    id: clearAllMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        historyModel.clear()
                        lineSeries.clear()
                        anomalySeries.clear()
                        statusText.text = ""
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                id: statusText
                font.family: Theme.fontFamily
                color: Theme.textMuted
                font.pixelSize: Theme.fontXs
            }

            Label {
                text: historyModel.pointCount + " points"
                font.family: Theme.fontFamilyMono
                color: Theme.textMuted
                font.pixelSize: Theme.fontXs
            }
        }
    }

    function loadHistory() {
        if (deviceCombo.currentIndex < 0) return

        loading = true
        historyModel.clear()
        lineSeries.clear()
        anomalySeries.clear()
        statusText.text = "Loading…"

        var device = deviceModel.data(
            deviceModel.index(deviceCombo.currentIndex, 0),
            deviceModel.DeviceIdRole)
        var sensor = sensorCombo.currentText
        var since = fromDate.text

        apiClient.fetchHistory(device, sensor, since)
    }
}
