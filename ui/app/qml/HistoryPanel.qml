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
    // ISO 8601 start of the query window, driven by the range presets and
    // passed to the API. Displayed to the user in a friendly form.
    property string sinceIso: {
        var d = new Date()
        d.setHours(d.getHours() - 24)
        return d.toISOString()
    }
    property double minVal: 0
    property double maxVal: 0
    property double minTs: 0
    property double maxTs: 0

    // Fit both axes to the loaded data extents (10% vertical margin). Used on
    // load and by the Reset control to undo any pan/zoom.
    function fitAxes() {
        if (historyModel.point_count() <= 0)
            return
        var margin = (root.maxVal - root.minVal) * 0.1
        if (margin === 0)
            margin = 1
        axisY.min = root.minVal - margin
        axisY.max = root.maxVal + margin
        axisX.min = new Date(root.minTs)
        axisX.max = new Date(root.maxTs)
    }

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
            root.fitAxes()
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
                                root.sinceIso = d.toISOString()
                                loadHistory()
                            }
                        }
                    }
                }

                // Friendly, read-only display of the query window start.
                // The range presets above drive it; showing the raw ISO string
                // here read as unfinished.
                Rectangle {
                    height: 30
                    Layout.preferredWidth: 190
                    radius: Theme.radiusMd
                    color: Theme.bgInput
                    border.color: Theme.border
                    border.width: 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spaceMd
                        anchors.rightMargin: Theme.spaceMd
                        spacing: 6
                        Icon {
                            name: "clock"
                            width: 12; height: 12
                            color: Theme.textMuted
                        }
                        Label {
                            text: {
                                var lbl = timeFormat.dateTimeLabel(root.sinceIso)
                                return lbl.length > 0 ? "From " + lbl : "From " + root.sinceIso
                            }
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontXs
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
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
                text: "Scroll to zoom · drag to pan"
                font.family: Theme.fontFamily
                font.pixelSize: 10
                color: Theme.textMuted
                opacity: 0.7
            }

            // Reset the pan/zoom back to the full loaded window. z > the pan
            // overlay so the button still receives clicks.
            Rectangle {
                z: 2
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.spaceMd
                visible: historyModel.pointCount > 0
                height: 28
                width: resetRow.implicitWidth + 18
                radius: Theme.radiusMd
                color: resetMouse.containsMouse ? Theme.bgCardHover : Theme.bgElevated
                border.color: Theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                RowLayout {
                    id: resetRow
                    anchors.centerIn: parent
                    spacing: 5
                    Icon { name: "chart"; width: 11; height: 11; color: Theme.textSecondary }
                    Label {
                        text: "Reset view"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXs
                        color: Theme.textSecondary
                    }
                }
                MouseArea {
                    id: resetMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.fitAxes()
                }
            }

            // Pan (drag) + cursor-centered zoom (wheel) on the time axis. The
            // range math lives in the tested ChartZoom helper; here we only map
            // pixels to a fraction of the plot width and feed axisX its result.
            // Y stays fit-to-data. Anchored to the ChartView so plotArea coords
            // line up with the mouse coords.
            MouseArea {
                anchors.fill: chartView
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                property real lastX: 0

                onPressed: function(mouse) { lastX = mouse.x }

                onPositionChanged: function(mouse) {
                    if (!pressed || historyModel.pointCount === 0)
                        return
                    var plotW = chartView.plotArea.width
                    if (plotW <= 0)
                        return
                    // Drag right → move the window back in time (content follows
                    // the cursor), so the pan fraction is the negative dx.
                    var dxFrac = -(mouse.x - lastX) / plotW
                    lastX = mouse.x
                    var r = chartZoom.pan(axisX.min.getTime(), axisX.max.getTime(), dxFrac)
                    axisX.min = new Date(r.min)
                    axisX.max = new Date(r.max)
                }

                onWheel: function(wheel) {
                    if (historyModel.pointCount === 0)
                        return
                    var plotW = chartView.plotArea.width
                    var focal = plotW > 0
                        ? (wheel.x - chartView.plotArea.x) / plotW
                        : 0.5
                    var scale = wheel.angleDelta.y > 0 ? 0.8 : 1.25
                    var r = chartZoom.zoom(axisX.min.getTime(), axisX.max.getTime(), focal, scale)
                    axisX.min = new Date(r.min)
                    axisX.max = new Date(r.max)
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

            // Range readout for the loaded window.
            Label {
                visible: historyModel.pointCount > 0
                text: "min " + root.minVal.toFixed(1) + "  ·  max " + root.maxVal.toFixed(1)
                font.family: Theme.fontFamilyMono
                color: Theme.textSecondary
                font.pixelSize: Theme.fontXs
            }

            Rectangle {
                visible: historyModel.pointCount > 0
                width: 1; height: 16
                color: Theme.divider
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

        // The combo's textRole is already "deviceId", so currentText is the
        // selected device id. (Don't resolve it via deviceModel.DeviceIdRole —
        // that role enum isn't Q_ENUM-exposed to QML, so it reads back as
        // undefined and the fetch goes out with an empty device id.)
        var device = deviceCombo.currentText
        var sensor = sensorCombo.currentText

        apiClient.fetchHistory(device, sensor, root.sinceIso)
    }
}
