import QtQuick

// Soft coloured halo that breathes behind an element — Motion Primitives'
// Glow Effect, built from stacked translucent rounded rects (no blur
// shader needed). Place it as a sibling *behind* the element it lights.
Item {
    id: root

    property real radius: Theme.radiusLg
    property color color: Theme.accent
    property real intensity: 1.0
    property bool breathing: true

    property real _pulse: 1.0
    SequentialAnimation on _pulse {
        running: root.breathing && root.visible && !Theme.reducedMotion
        loops: Animation.Infinite
        NumberAnimation { from: 1.0; to: 0.55; duration: Theme.motionLoop; easing.type: Easing.InOutSine }
        NumberAnimation { from: 0.55; to: 1.0; duration: Theme.motionLoop; easing.type: Easing.InOutSine }
    }

    Repeater {
        // spread (px) / alpha pairs, outermost first
        model: [[16, 0.035], [10, 0.06], [6, 0.10], [3, 0.16]]
        Rectangle {
            anchors.fill: parent
            anchors.margins: -modelData[0]
            radius: root.radius + modelData[0]
            color: Theme.alpha(root.color, modelData[1] * root.intensity * root._pulse)
        }
    }
}
