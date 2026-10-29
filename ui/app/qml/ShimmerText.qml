import QtQuick

// Text with a soft highlight sweeping across it (Motion Primitives' Text
// Shimmer) — for "connecting…" / "loading…" states. The highlight is the
// same text drawn brighter inside three nested clip windows of rising
// opacity, which approximates a gradient mask without a shader.
Item {
    id: root

    property alias text: base.text
    property alias font: base.font
    property color color: Theme.textMuted
    property color shineColor: Theme.textPrimary
    property bool running: true
    property int duration: 1900

    property real sweep: -0.3
    readonly property real _band: Math.max(48, width * 0.4)

    implicitWidth: base.implicitWidth
    implicitHeight: base.implicitHeight

    NumberAnimation on sweep {
        from: -0.3; to: 1.3
        duration: root.duration
        loops: Animation.Infinite
        running: root.running && root.visible && !Theme.reducedMotion
    }

    Text {
        id: base
        color: root.color
        font.family: Theme.fontFamily
    }

    Repeater {
        // band width fraction, opacity
        model: root.running && !Theme.reducedMotion ? [[1.0, 0.3], [0.55, 0.55], [0.22, 0.9]] : []
        Item {
            readonly property real w: root._band * modelData[0]
            x: root.sweep * root.width - w / 2
            width: w
            height: root.height
            clip: true

            Text {
                x: -parent.x
                text: base.text
                font: base.font
                color: root.shineColor
                opacity: modelData[1]
            }
        }
    }
}
