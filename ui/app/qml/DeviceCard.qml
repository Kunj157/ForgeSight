import QtQuick
import QtQuick.Layouts

// One live sensor tile: identity, status pill, an easing value that tints
// on change, trend chip, rolling sparkline and range. Critical tiles get a
// red border trail and a pulsing pill. Fades and rises in when created
// (staggered by `revealIndex`).
GlowCard {
    id: card

    property string deviceId: ""
    property string sensor: ""
    property real value: 0
    property string unit: ""
    property string status: "normal"
    property string timestamp: ""
    property int tick: 0                 // bumps periodically so "updated" text ages
    property int revealIndex: 0
    property int historyLength: 24
    // Set by the panel from the scroll position. Off-screen cards keep
    // recording history but skip animation, sparkline geometry and stats,
    // which keeps 600 live tiles affordable.
    property bool inView: true
    property bool selected: false
    signal activated()
    signal locateRequested()

    readonly property bool critical: status === "critical"
    readonly property color tone: critical ? Theme.critical : Theme.accent

    property real riseY: 0
    property var history: []
    property var stats: ({ valid: false })

    function record(v) {
        var next = history.slice()
        next.push(v)
        if (next.length > historyLength)
            next.shift()
        history = next
        if (inView)
            stats = seriesStats.compute(next)
    }
    onInViewChanged: if (inView) stats = seriesStats.compute(history)
    onValueChanged: record(value)
    Component.onCompleted: {
        record(value)
        if (!Theme.reducedMotion && revealIndex < 24)
            reveal.start()
        else
            opacity = 1
    }

    opacity: 0
    trail: critical && inView
    borderColor: selected ? Theme.accent
               : critical ? Theme.alpha(Theme.critical, 0.45)
               : hovered ? Theme.borderStrong : Theme.border
    glow: selected
    glowColor: Theme.accent

    TapHandler { onTapped: card.activated() }
    spotlightColor: tone

    SequentialAnimation {
        id: reveal
        PauseAnimation { duration: Math.max(0, card.revealIndex) * 28 }
        ParallelAnimation {
            NumberAnimation { target: card; property: "opacity"; from: 0; to: 1; duration: 320; easing.type: Easing.OutCubic }
            NumberAnimation { target: card; property: "riseY"; from: 10; to: 0; duration: 420; easing.type: Easing.OutCubic }
        }
    }

    ColumnLayout {
        visible: card.inView
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        anchors.bottomMargin: Theme.spaceMd
        spacing: 6
        transform: Translate { y: card.riseY }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            Rectangle {
                implicitWidth: 28; implicitHeight: 28
                radius: Theme.radiusSm
                color: Theme.alpha(card.critical ? Theme.critical : Theme.textMuted, 0.12)
                Icon {
                    anchors.centerIn: parent
                    width: 14; height: 14
                    name: card.sensor === "vibration" ? "activity" : "gauge"
                    color: card.critical ? Theme.critical : Theme.textSecondary
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    text: card.deviceId
                    elide: Text.ElideRight
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontMd
                    font.bold: true
                    color: Theme.textPrimary
                }
                Text {
                    text: card.sensor
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontXs
                    color: Theme.textMuted
                }
            }
            Rectangle {
                visible: card.selected
                implicitWidth: 24; implicitHeight: 24
                radius: 12
                color: locateMouse.containsMouse ? Theme.accent : Theme.accentSoft
                Icon { anchors.centerIn: parent; name: "cube"; width: 13; height: 13; color: locateMouse.containsMouse ? "#ffffff" : Theme.accent }
                MouseArea {
                    id: locateMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: card.locateRequested()
                }
            }
            StatusPill {
                status: card.status
                pulse: card.critical && card.inView
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AnimatedNumber {
                value: card.value
                flashOnChange: true
                animated: card.inView
                baseColor: card.critical ? Theme.critical : Theme.textPrimary
                font.pixelSize: Theme.fontDisplay
                font.bold: true
                Layout.alignment: Qt.AlignBaseline
            }
            Text {
                text: card.unit
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSm
                color: Theme.textMuted
                Layout.alignment: Qt.AlignBaseline
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                visible: card.stats.valid && card.stats.direction !== "flat"
                implicitHeight: 20
                implicitWidth: trendRow.implicitWidth + 12
                radius: 10
                color: Theme.bgElevated
                Row {
                    id: trendRow
                    anchors.centerIn: parent
                    spacing: 3
                    Text {
                        text: card.stats.direction === "up" ? "▲" : "▼"
                        font.pixelSize: 8
                        color: card.stats.direction === "up" ? Theme.success : Theme.info
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: card.stats.valid ? Math.abs(card.stats.delta).toFixed(1) : ""
                        font.family: Theme.fontFamilyMono
                        font.pixelSize: Theme.fontXs
                        color: Theme.textSecondary
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        Sparkline {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 30
            values: card.inView ? card.history : []
            color: card.tone
        }

        RowLayout {
            Layout.fillWidth: true
            Text {
                visible: card.stats.valid && card.stats.count > 1
                text: card.stats.valid ? "min " + card.stats.min.toFixed(1) + "  ·  max " + card.stats.max.toFixed(1) : ""
                font.family: Theme.fontFamilyMono
                font.pixelSize: 10
                color: Theme.textMuted
            }
            Item { Layout.fillWidth: true }
            Text {
                text: {
                    card.tick
                    var rel = timeFormat.relative(card.timestamp)
                    return rel.length > 0 ? rel : ""
                }
                font.family: Theme.fontFamily
                font.pixelSize: 10
                color: Theme.textMuted
            }
        }
    }
}
