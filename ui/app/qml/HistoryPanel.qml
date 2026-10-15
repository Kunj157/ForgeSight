import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

Rectangle {
    id: root
    color: "transparent"

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
    property double sum: 0
    property int anomalies: 0
    readonly property double avgVal: historyModel.pointCount > 0 ? sum / historyModel.pointCount : 0
    property string unit: ""

    // Loaded samples, kept for the hover crosshair's nearest-point lookup.
    property var tsList: []
    property var valList: []

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

    function resetData() {
        historyModel.clear()
        lineSeries.clear()
        anomalySeries.clear()
        sum = 0
        anomalies = 0
        tsList = []
        valList = []
    }

    // Index of the loaded sample closest in time to ts (binary search; the
    // API returns samples in time order).
    function nearestIndex(ts) {
        var lo = 0, hi = tsList.length - 1
        if (hi < 0) return -1
        while (hi - lo > 1) {
            var mid = (lo + hi) >> 1
            if (tsList[mid] < ts) lo = mid
            else hi = mid
        }
        return Math.abs(tsList[lo] - ts) <= Math.abs(tsList[hi] - ts) ? lo : hi
    }

    Connections {
        target: apiClient
        function onHistoryPointReceived(deviceId, sensor, value, unit, timestamp, anomaly) {
            historyModel.add_point(deviceId, sensor, value, unit, timestamp, anomaly)
            var ts = Date.parse(timestamp)
            lineSeries.append(ts, value)
            if (anomaly) {
                anomalySeries.append(ts, value)
                root.anomalies++
            }
            root.tsList.push(ts)
            root.valList.push(value)
            root.sum += value
            root.unit = unit

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

    component StyledCombo: ComboBox {
        id: combo
        implicitHeight: 34
        contentItem: Text {
            text: combo.currentText
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            leftPadding: 12
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            color: Theme.bgInput
            radius: Theme.radiusMd
            border.width: 1
            border.color: combo.pressed || combo.popup.visible ? Theme.accent
                        : combo.hovered ? Theme.borderStrong : Theme.border
            Behavior on border.color { ColorAnimation { duration: Theme.motionFast } }
        }
        indicator: Icon {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 12
            width: 10; height: 10
            name: "chevron-down"
            color: Theme.textMuted
            rotation: combo.popup.visible ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: Theme.motionMed } }
        }
        delegate: Component {
            ItemDelegate {
                width: combo.width
                highlighted: combo.highlightedIndex === index
                contentItem: Text {
                    text: combo.textRole ? (Array.isArray(combo.model) ? modelData[combo.textRole] : model[combo.textRole]) : modelData
                    color: highlighted ? Theme.textPrimary : Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSm
                }
                background: Rectangle {
                    color: highlighted ? Theme.accentSoft : "transparent"
                    radius: Theme.radiusSm
                }
            }
        }
        popup.background: Rectangle {
            color: Theme.bgElevated
            radius: Theme.radiusMd
            border.color: Theme.borderStrong
        }
    }

    component Stat: Column {
        property string label: ""
        property real value: 0
        property int decimals: 1
        property color tone: Theme.textPrimary
        spacing: 2
        Text {
            text: parent.label
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.bold: true
            font.letterSpacing: Theme.trackingCaps
            color: Theme.textMuted
        }
        AnimatedNumber {
            value: parent.value
            decimals: parent.decimals
            baseColor: parent.tone
            font.pixelSize: Theme.fontXl
            font.bold: true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceLg

        // —— Query toolbar ——
        GlowCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            interactive: false
            elevation: 2

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceLg
                anchors.rightMargin: Theme.spaceLg
                spacing: Theme.spaceMd

                StyledCombo {
                    id: deviceCombo
                    model: deviceModel
                    textRole: "deviceId"
                    Layout.preferredWidth: 170
                }
                StyledCombo {
                    id: sensorCombo
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

                Rectangle { width: 1; height: 24; color: Theme.divider }

                SegmentedControl {
                    currentValue: root.selectedRangeHours
                    model: [
                        { label: "1H", value: 1 },
                        { label: "6H", value: 6 },
                        { label: "24H", value: 24 },
                        { label: "7D", value: 168 },
                    ]
                    onActivated: function(hours) {
                        root.selectedRangeHours = hours
                        var d = new Date()
                        d.setHours(d.getHours() - hours)
                        root.sinceIso = d.toISOString()
                        loadHistory()
                    }
                }

                // Friendly, read-only display of the query window start.
                Row {
                    spacing: 6
                    Icon { name: "clock"; width: 12; height: 12; color: Theme.textMuted; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: {
                            var lbl = timeFormat.dateTimeLabel(root.sinceIso)
                            return lbl.length > 0 ? "From " + lbl : "From " + root.sinceIso
                        }
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXs
                        color: Theme.textSecondary
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Item { Layout.fillWidth: true }

                FxButton {
                    text: "Load"
                    icon: "refresh"
                    variant: "primary"
                    busy: root.loading
                    implicitWidth: 92
                    onClicked: loadHistory()
                }
            }
        }

        // —— Stats strip ——
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2xl
            visible: historyModel.pointCount > 0
            Stat { label: "MIN"; value: root.minVal }
            Stat { label: "AVG"; value: root.avgVal }
            Stat { label: "MAX"; value: root.maxVal }
            Stat { label: "POINTS"; value: historyModel.pointCount; decimals: 0 }
            Stat {
                label: "ANOMALIES"; value: root.anomalies; decimals: 0
                tone: root.anomalies > 0 ? Theme.critical : Theme.textPrimary
            }
            Item { Layout.fillWidth: true }
            Row {
                spacing: Theme.spaceLg
                Row {
                    spacing: 6
                    Rectangle { width: 14; height: 3; radius: 1.5; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: deviceCombo.currentText + " · " + sensorCombo.currentText; font.family: Theme.fontFamily; font.pixelSize: Theme.fontXs; color: Theme.textSecondary }
                }
                Row {
                    spacing: 6
                    Rectangle { width: 8; height: 8; radius: 4; color: Theme.critical; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "anomaly"; font.family: Theme.fontFamily; font.pixelSize: Theme.fontXs; color: Theme.textSecondary }
                }
            }
        }

        // —— Chart ——
        GlowCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            interactive: false

            ChartView {
                id: chartView
                anchors.fill: parent
                anchors.margins: 6
                antialiasing: true
                backgroundColor: "transparent"
                plotAreaColor: "transparent"
                legend.visible: false
                margins.top: 12
                margins.bottom: 6
                margins.left: 6
                margins.right: 12

                DateTimeAxis {
                    id: axisX
                    format: root.selectedRangeHours > 24 ? "MMM d HH:mm" : "HH:mm"
                    labelsColor: Theme.alpha(Theme.textSecondary, 0.8)
                    labelsFont.family: Theme.fontFamilyMono
                    labelsFont.pixelSize: 10
                    gridLineColor: Theme.divider
                    minorGridVisible: false
                    tickCount: 7
                    color: Theme.border
                }

                ValueAxis {
                    id: axisY
                    labelsColor: Theme.alpha(Theme.textSecondary, 0.8)
                    labelsFont.family: Theme.fontFamilyMono
                    labelsFont.pixelSize: 10
                    gridLineColor: Theme.divider
                    labelFormat: "%.1f"
                    tickCount: 6
                    color: "transparent"
                }

                AreaSeries {
                    name: "Readings area"
                    axisX: axisX
                    axisY: axisY
                    color: Theme.alpha(Theme.accent, Theme.dark ? 0.12 : 0.10)
                    borderColor: "transparent"
                    borderWidth: 0
                    upperSeries: LineSeries { id: areaEdge }
                }

                LineSeries {
                    id: lineSeries
                    name: "Readings"
                    color: Theme.accent
                    width: 2
                    axisX: axisX
                    axisY: axisY
                    onPointAdded: function(i) { var p = at(i); areaEdge.append(p.x, p.y) }
                    onPointsRemoved: areaEdge.clear()
                }

                ScatterSeries {
                    id: anomalySeries
                    name: "Anomalies"
                    color: Theme.critical
                    markerSize: 9
                    borderColor: Theme.bgCard
                    borderWidth: 2
                    axisX: axisX
                    axisY: axisY
                }
            }

            // Hover crosshair + readout.
            Item {
                id: crosshair
                anchors.fill: chartView
                visible: pan.containsMouse && !pan.pressed && hoverIndex >= 0
                property int hoverIndex: -1
                property point at: Qt.point(0, 0)

                Rectangle {
                    x: crosshair.at.x
                    y: chartView.plotArea.y
                    width: 1
                    height: chartView.plotArea.height
                    color: Theme.alpha(Theme.textSecondary, 0.35)
                }
                Rectangle {
                    x: crosshair.at.x - 5
                    y: crosshair.at.y - 5
                    width: 10; height: 10; radius: 5
                    color: Theme.bgCard
                    border.width: 2
                    border.color: Theme.accent
                }
                GlowCard {
                    interactive: false
                    elevation: 2
                    width: readout.implicitWidth + 24
                    height: readout.implicitHeight + 16
                    x: Math.min(crosshair.at.x + 14, crosshair.width - width - 4)
                    y: Math.max(chartView.plotArea.y, crosshair.at.y - height - 12)
                    Column {
                        id: readout
                        anchors.centerIn: parent
                        spacing: 2
                        Text {
                            text: crosshair.hoverIndex >= 0
                                  ? root.valList[crosshair.hoverIndex].toFixed(2) + " " + root.unit : ""
                            font.family: Theme.fontFamilyMono
                            font.pixelSize: Theme.fontMd
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Text {
                            text: crosshair.hoverIndex >= 0
                                  ? Qt.formatDateTime(new Date(root.tsList[crosshair.hoverIndex]), "MMM d, HH:mm:ss") : ""
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            color: Theme.textMuted
                        }
                    }
                }
            }

            Text {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Theme.spaceMd
                visible: historyModel.pointCount > 0
                text: "Scroll to zoom · drag to pan"
                font.family: Theme.fontFamily
                font.pixelSize: 10
                color: Theme.textMuted
            }

            // Reset the pan/zoom back to the full loaded window. z > the pan
            // overlay so the button still receives clicks.
            FxButton {
                z: 2
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.spaceMd
                visible: historyModel.pointCount > 0
                text: "Reset view"
                icon: "chart"
                compact: true
                onClicked: root.fitAxes()
            }

            // Pan (drag) + cursor-centered zoom (wheel) on the time axis. The
            // range math lives in the tested ChartZoom helper; here we only map
            // pixels to a fraction of the plot width and feed axisX its result.
            // Y stays fit-to-data. Hover drives the crosshair.
            MouseArea {
                id: pan
                anchors.fill: chartView
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.CrossCursor
                property real lastX: 0

                onPressed: function(mouse) { lastX = mouse.x }

                onPositionChanged: function(mouse) {
                    if (historyModel.pointCount === 0)
                        return
                    var plotW = chartView.plotArea.width
                    if (plotW <= 0)
                        return
                    if (!pressed) {
                        var v = chartView.mapToValue(Qt.point(mouse.x, mouse.y), lineSeries)
                        var i = root.nearestIndex(v.x)
                        crosshair.hoverIndex = i
                        if (i >= 0)
                            crosshair.at = chartView.mapToPosition(Qt.point(root.tsList[i], root.valList[i]), lineSeries)
                        return
                    }
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

            // Loading skeleton
            Item {
                anchors.fill: parent
                anchors.margins: Theme.spaceXl
                visible: root.loading
                Column {
                    anchors.fill: parent
                    spacing: (height - 6 * 10) / 5
                    Repeater {
                        model: 6
                        Skeleton { width: parent.width * (0.55 + 0.45 * ((index * 37) % 10) / 10); height: 10 }
                    }
                }
                ShimmerText {
                    anchors.centerIn: parent
                    text: "Loading " + deviceCombo.currentText + " · " + sensorCombo.currentText + "…"
                    font.pixelSize: Theme.fontMd
                }
            }

            EmptyState {
                anchors.centerIn: parent
                visible: historyModel.pointCount === 0 && !root.loading
                icon: "chart"
                title: "No history loaded"
                subtitle: "Pick a device, sensor and range, then Load."
                actionText: deviceCombo.currentIndex >= 0 ? "Load last " + (root.selectedRangeHours > 24 ? "7 days" : root.selectedRangeHours + "h") : ""
                onAction: loadHistory()
            }
        }

        // —— Footer actions ——
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            FxButton {
                text: "Export CSV"
                icon: "download"
                enabled: historyModel.pointCount > 0
                onClicked: {
                    var path = historyModel.default_export_path("history_export.csv")
                    if (historyModel.export_csv(path)) {
                        statusText.text = "Exported to " + path
                        Window.window.toast("CSV exported", path, "ok")
                    } else {
                        statusText.text = "CSV export failed"
                    }
                }
            }
            FxButton {
                text: "Export PDF"
                icon: "download"
                enabled: historyModel.pointCount > 0
                onClicked: {
                    var path = historyModel.default_export_path("history_export.pdf")
                    if (historyModel.export_pdf(path, Window.window)) {
                        statusText.text = "Exported to " + path
                        Window.window.toast("PDF exported", path, "ok")
                    } else {
                        statusText.text = "PDF export failed"
                    }
                }
            }
            FxButton {
                text: "Clear"
                icon: "trash"
                enabled: historyModel.pointCount > 0
                onClicked: {
                    root.resetData()
                    statusText.text = ""
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                id: statusText
                font.family: Theme.fontFamily
                color: Theme.textMuted
                font.pixelSize: Theme.fontXs
            }
        }
    }

    function loadHistory() {
        if (deviceCombo.currentIndex < 0) return

        loading = true
        resetData()
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
