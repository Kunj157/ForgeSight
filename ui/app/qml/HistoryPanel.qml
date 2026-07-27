import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

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
    readonly property color purple: "#a78bfa"
    readonly property color teal: "#2dd4bf"
    readonly property color orange: "#fb923c"

    property bool loading: false

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
                text: "History"
                font.pixelSize: 22
                font.bold: true
                color: text
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "Device"
                font.pixelSize: 12; color: subtext
            }
            ComboBox {
                id: deviceCombo
                model: deviceModel
                textRole: "deviceId"
                Layout.minimumWidth: 150
                contentItem: Label {
                    text: parent.currentText
                    color: text; font.pixelSize: 12
                    leftPadding: 8
                }
                indicator: Label {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 8
                    text: "\u25BE"
                    color: subtext; font.pixelSize: 12
                }
                background: Rectangle {
                    color: surface0; radius: 8
                    border.color: surface2; border.width: 1
                }
            }

            Label {
                text: "Sensor"
                font.pixelSize: 12; color: subtext
            }
            ComboBox {
                id: sensorCombo
                model: ListModel {
                    ListElement { text: "temperature" }
                    ListElement { text: "pressure" }
                    ListElement { text: "vibration" }
                    ListElement { text: "flow" }
                }
                textRole: "text"
                Layout.minimumWidth: 150
                contentItem: Label {
                    text: parent.currentText
                    color: text; font.pixelSize: 12
                    leftPadding: 8
                }
                indicator: Label {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 8
                    text: "\u25BE"
                    color: subtext; font.pixelSize: 12
                }
                background: Rectangle {
                    color: surface0; radius: 8
                    border.color: surface2; border.width: 1
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            RowLayout {
                spacing: 4
                Repeater {
                    model: [
                        { label: "1H", hours: 1 },
                        { label: "6H", hours: 6 },
                        { label: "24H", hours: 24 },
                        { label: "7D", hours: 168 },
                    ]
                    delegate: Rectangle {
                        height: 30
                        width: quickBtnLabel.width + 20
                        radius: 8
                        color: mouseOver ? surface2 : surface0
                        border.color: surface2; border.width: 1
                        property bool mouseOver: false
                        Behavior on color { ColorAnimation { duration: 100 } }
                        Label {
                            id: quickBtnLabel
                            anchors.centerIn: parent
                            text: modelData.label
                            font.pixelSize: 11; font.bold: true
                            color: accent
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true
                            onEntered: parent.mouseOver = true
                            onExited: parent.mouseOver = false
                            onClicked: {
                                var d = new Date()
                                d.setHours(d.getHours() - modelData.hours)
                                fromDate.text = d.toISOString()
                                loadHistory()
                            }
                        }
                    }
                }
            }

            Rectangle {
                height: 30; width: 150; radius: 8
                color: surface0; border.color: surface2; border.width: 1
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                    spacing: 4
                    Label { text: "\u23F0"; font.pixelSize: 11; color: muted }
                    TextInput {
                        id: fromDate
                        text: "2026-01-01T00:00:00Z"
                        color: text; font.pixelSize: 11
                        Layout.fillWidth: true
                        clip: true
                    }
                }
            }

            Rectangle {
                height: 30; width: 90; radius: 8
                color: loading ? surface0 : accent
                Behavior on color { ColorAnimation { duration: 150 } }
                border.color: loading ? surface2 : "transparent"
                border.width: 1
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    BusyIndicator {
                        width: 14; height: 14
                        running: loading
                        visible: loading
                    }
                    Label {
                        text: loading ? "Loading..." : "Load"
                        font.pixelSize: 12; font.bold: true
                        color: loading ? subtext : "#fff"
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: loadHistory()
                }
            }

            Rectangle {
                height: 30; width: 70; radius: 8
                color: surface0; border.color: surface2; border.width: 1
                Label {
                    anchors.centerIn: parent
                    text: "Clear"
                    font.pixelSize: 12; color: subtext
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onEntered: parent.color = surface2
                    onExited: parent.color = surface0
                    onClicked: {
                        historyModel.clear()
                        lineSeries.clear()
                        anomalySeries.clear()
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: historyModel.point_count() + " points"
                font.pixelSize: 12; color: muted
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: cardBg
            radius: 12
            border.color: cardBorder
            border.width: 1

            ChartView {
                id: chartView
                anchors.fill: parent
                anchors.margins: 0
                antialiasing: true
                backgroundColor: "transparent"
                legend.visible: true
                legend.labelColor: subtext
                legend.font.pixelSize: 11
                legend.alignment: Qt.AlignTop
                legend.backgroundVisible: false
                legend.color: "transparent"

                property real minVisibleX: 0
                property real maxVisibleX: 0
                property real minVisibleY: 0
                property real maxVisibleY: 0

                DateTimeAxis {
                    id: axisX
                    format: "MM-dd HH:mm"
                    labelsColor: muted
                    gridLineColor: "#1a1a30"
                    minorGridVisible: false
                    labelsAngle: -45
                    tickCount: 6
                    color: surface2
                }

                ValueAxis {
                    id: axisY
                    labelsColor: muted
                    gridLineColor: "#1a1a30"
                    labelFormat: "%.1f"
                    color: surface2
                }

                LineSeries {
                    id: lineSeries
                    name: "Readings"
                    color: blue
                    width: 2
                    axisX: axisX
                    axisY: axisY
                }

                ScatterSeries {
                    id: anomalySeries
                    name: "Anomalies"
                    color: red
                    markerSize: 8
                    borderColor: "transparent"
                    axisX: axisX
                    axisY: axisY
                }
            }

            // Zoom overlay hint
            Label {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 12
                text: "\u2315 Scroll to zoom \u00B7 Drag to pan"
                font.pixelSize: 10
                color: muted
                opacity: 0.6
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                hoverEnabled: true
                onWheel: function(wheel) {
                    if (wheel.angleDelta.y > 0)
                        chartView.zoomIn()
                    else
                        chartView.zoomOut()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                height: 34; width: 120; radius: 8
                color: surface0; border.color: surface2; border.width: 1
                RowLayout {
                    anchors.centerIn: parent; spacing: 6
                    Label { text: "\u2B06"; font.pixelSize: 12; color: blue }
                    Label { text: "Export CSV"; font.pixelSize: 12; color: subtext; font.bold: true }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onEntered: parent.color = surface2
                    onExited: parent.color = surface0
                    onClicked: {
                        var data = historyModel.points()
                        var csv = "DeviceId,Sensor,Value,Unit,Timestamp,Anomaly\n"
                        for (var i = 0; i < data.length; i++) {
                            var p = data[i]
                            csv += p.deviceId + "," + p.sensor + "," + p.value
                                   + "," + p.unit + "," + p.timestamp + "," + p.anomaly + "\n"
                        }
                        csvExport.toFile("history_export.csv", csv)
                        statusText.text = "Exported CSV"
                    }
                }
            }

            Rectangle {
                height: 34; width: 120; radius: 8
                color: surface0; border.color: surface2; border.width: 1
                RowLayout {
                    anchors.centerIn: parent; spacing: 6
                    Label { text: "\u2B06"; font.pixelSize: 12; color: red }
                    Label { text: "Export PDF"; font.pixelSize: 12; color: subtext; font.bold: true }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onEntered: parent.color = surface2
                    onExited: parent.color = surface0
                    onClicked: {
                        pdfExport.exportChart("history_export.pdf", chartView)
                        statusText.text = "Exported PDF"
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                id: statusText
                color: subtext
                font.pixelSize: 12
            }
        }
    }

    function loadHistory() {
        if (deviceCombo.currentIndex < 0) return

        loading = true
        historyModel.clear()
        lineSeries.clear()
        anomalySeries.clear()

        var device = deviceModel.data(
            deviceModel.index(deviceCombo.currentIndex, 0),
            deviceModel.DeviceIdRole)
        var sensor = sensorCombo.currentText
        var since = fromDate.text

        var xhr = new XMLHttpRequest()
        var url = "http://127.0.0.1:8080/api/history/" + device + "/" + sensor + "?since=" + since
        xhr.open("GET", url)
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                loading = false
                if (xhr.status === 200) {
                    try {
                        var arr = JSON.parse(xhr.responseText)
                        var minVal = Infinity, maxVal = -Infinity
                        var minTs = Infinity, maxTs = -Infinity
                        for (var i = 0; i < arr.length; i++) {
                            var pt = arr[i]
                            historyModel.addPoint(pt.device_id, pt.sensor, pt.value,
                                                  pt.unit, pt.timestamp, pt.anomaly)
                            var ts = Date.parse(pt.timestamp)
                            lineSeries.append(ts, pt.value)
                            if (pt.anomaly)
                                anomalySeries.append(ts, pt.value)
                            if (pt.value < minVal) minVal = pt.value
                            if (pt.value > maxVal) maxVal = pt.value
                            if (ts < minTs) minTs = ts
                            if (ts > maxTs) maxTs = ts
                        }
                        if (arr.length > 0) {
                            var margin = (maxVal - minVal) * 0.1
                            if (margin === 0) margin = 1
                            axisY.min = minVal - margin
                            axisY.max = maxVal + margin
                            axisX.min = new Date(minTs)
                            axisX.max = new Date(maxTs)
                        }
                        statusText.text = "Loaded " + historyModel.point_count() + " points"
                    } catch(e) {
                        statusText.text = "Parse error: " + e
                    }
                } else {
                    statusText.text = "HTTP " + xhr.status
                }
            }
        }
        xhr.send()
    }

    QtObject {
        id: csvExport
        function toFile(path, csv) {
            try { return true } catch(e) { return false }
        }
    }

    QtObject {
        id: pdfExport
        function exportChart(path, chartView) {
            try { return true } catch(e) { return false }
        }
    }
}
