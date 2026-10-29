import QtQuick

// Placeholder block with a light band sweeping across it, shown while data
// is loading so layout doesn't jump when it arrives.
Rectangle {
    id: root

    property bool running: true
    property real phase: 0

    radius: Theme.radiusSm
    color: Theme.bgElevated
    clip: true

    NumberAnimation on phase {
        from: 0; to: 1
        duration: 1500
        loops: Animation.Infinite
        running: root.running && root.visible && !Theme.reducedMotion
    }

    Rectangle {
        readonly property real w: Math.max(60, root.width * 0.55)
        x: -w + root.phase * (root.width + w)
        width: w
        height: root.height
        visible: !Theme.reducedMotion
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.5; color: Theme.dark ? "#0fffffff" : "#99ffffff" }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }
}
