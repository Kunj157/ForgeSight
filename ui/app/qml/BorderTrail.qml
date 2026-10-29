import QtQuick
import QtQuick.Shapes

// A short comet of light that travels around a rounded-rect border — the
// QML take on Motion Primitives' Border Trail. Drawn as dashed strokes on
// the same rounded-rect path, so it follows the corners exactly with no
// clipping or masking. Three stacked dashes (long + faint, mid, short +
// bright) share a leading edge, which reads as a fading tail.
Item {
    id: root

    property real radius: Theme.radiusLg
    property color color: Theme.critical
    // Fraction of the perimeter covered by the tail.
    property real length: 0.22
    property int duration: 3600
    property bool running: true
    property real lineWidth: 1.6

    // 0..1 position of the head along the perimeter.
    property real progress: 0

    readonly property real _r: Math.min(radius, width / 2, height / 2)
    readonly property real perimeter:
        2 * (width + height) - 8 * _r + 2 * Math.PI * _r
    readonly property bool _active: running && width > 0 && !Theme.reducedMotion

    NumberAnimation on progress {
        from: 0; to: 1
        duration: root.duration
        loops: Animation.Infinite
        running: root._active && root.visible
    }

    // dashPattern / dashOffset are in units of stroke width. Each layer's
    // dash ends at the shared head position.
    function pattern(frac, w) {
        var on = Math.max(1, perimeter * frac)
        return [on / w, Math.max(0.01, (perimeter - on)) / w]
    }
    function offset(frac, w) {
        var on = perimeter * frac
        var start = progress * perimeter - on
        return -start / w
    }

    visible: _active

    Shape {
        anchors.fill: parent
        antialiasing: true

        // Shared path geometry for every layer, inset by half a pixel so the
        // stroke sits on the card's border line.
        component Track: ShapePath {
            property real frac: 0.2
            property real w: root.lineWidth
            property real op: 1.0
            fillColor: "transparent"
            strokeColor: Theme.alpha(root.color, op)
            strokeWidth: w
            capStyle: ShapePath.RoundCap
            strokeStyle: ShapePath.DashLine
            dashPattern: root.pattern(frac, w)
            dashOffset: root.offset(frac, w)

            startX: root._r + 0.5; startY: 0.5
            PathLine { x: root.width - root._r - 0.5; y: 0.5 }
            PathArc { x: root.width - 0.5; y: root._r + 0.5; radiusX: root._r; radiusY: root._r }
            PathLine { x: root.width - 0.5; y: root.height - root._r - 0.5 }
            PathArc { x: root.width - root._r - 0.5; y: root.height - 0.5; radiusX: root._r; radiusY: root._r }
            PathLine { x: root._r + 0.5; y: root.height - 0.5 }
            PathArc { x: 0.5; y: root.height - root._r - 0.5; radiusX: root._r; radiusY: root._r }
            PathLine { x: 0.5; y: root._r + 0.5 }
            PathArc { x: root._r + 0.5; y: 0.5; radiusX: root._r; radiusY: root._r }
        }

        // Soft halo around the head.
        Track { frac: root.length * 0.35; w: root.lineWidth * 5; op: 0.12 }
        Track { frac: root.length; w: root.lineWidth; op: 0.28 }
        Track { frac: root.length * 0.5; w: root.lineWidth; op: 0.6 }
        Track { frac: root.length * 0.16; w: root.lineWidth * 1.4; op: 1.0 }
    }
}
