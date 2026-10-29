import QtQuick
import QtQuick.Shapes

// Cursor-following radial light clipped to a rounded rect — Motion
// Primitives' Spotlight. The fill is a RadialGradient on a rounded-rect
// ShapePath, so it respects the card's corners without a mask. Fades in and
// out with `active`; costs nothing while hidden.
Item {
    id: root

    property real radius: Theme.radiusLg
    property color color: Theme.accent
    property real size: 220
    property real strength: Theme.dark ? 0.17 : 0.10
    property bool active: false
    property point position: Qt.point(width / 2, height / 2)

    readonly property real _r: Math.min(radius, width / 2, height / 2)

    opacity: active ? 1 : 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: Theme.motionMed; easing.type: Easing.OutCubic } }

    Shape {
        anchors.fill: parent
        antialiasing: true

        ShapePath {
            strokeColor: "transparent"
            fillGradient: RadialGradient {
                centerX: root.position.x; centerY: root.position.y
                focalX: root.position.x; focalY: root.position.y
                centerRadius: root.size / 2
                GradientStop { position: 0.0; color: Theme.alpha(root.color, root.strength) }
                GradientStop { position: 0.55; color: Theme.alpha(root.color, root.strength * 0.35) }
                GradientStop { position: 1.0; color: Theme.alpha(root.color, 0) }
            }

            startX: root._r; startY: 0
            PathLine { x: root.width - root._r; y: 0 }
            PathArc { x: root.width; y: root._r; radiusX: root._r; radiusY: root._r }
            PathLine { x: root.width; y: root.height - root._r }
            PathArc { x: root.width - root._r; y: root.height; radiusX: root._r; radiusY: root._r }
            PathLine { x: root._r; y: root.height }
            PathArc { x: 0; y: root.height - root._r; radiusX: root._r; radiusY: root._r }
            PathLine { x: 0; y: root._r }
            PathArc { x: root._r; y: 0; radiusX: root._r; radiusY: root._r }
        }
    }
}
