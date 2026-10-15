import QtQuick

// The dashboard's surface primitive: a crisp card with a subtle vertical
// gradient, a hairline top highlight, layered soft elevation, and optional
// motion — hover spotlight, breathing glow and a border trail. Every
// optional effect sits behind a Loader, so a plain card (the common case,
// e.g. 600 device tiles) costs a handful of rectangles.
Item {
    id: root

    default property alias content: contentItem.data

    property real radius: Theme.radiusLg
    property color color: Theme.bgCard
    property color colorTop: Theme.bgCardTop
    property color borderColor: hovered ? Theme.borderStrong : Theme.border
    // 0 = flat, 1 = resting card, 2 = raised (popovers, toolbars)
    property int elevation: 1
    property bool interactive: true
    property bool lift: interactive

    property bool spotlight: interactive
    property color spotlightColor: Theme.accent
    property bool glow: false
    property color glowColor: Theme.accent
    property bool trail: false
    property color trailColor: Theme.critical

    readonly property bool hovered: hover.hovered
    readonly property point pointer: hover.point.position

    HoverHandler { id: hover; enabled: root.interactive }

    transform: Translate {
        y: root.lift && root.hovered ? -2 : 0
        Behavior on y { NumberAnimation { duration: Theme.motionMed; easing.type: Easing.OutCubic } }
    }

    // Elevation: stacked low-alpha offsets read as a soft shadow without a
    // blur pass. Stronger while hovered.
    Repeater {
        model: root.elevation > 0 ? 3 : 0
        Rectangle {
            readonly property real spread: (index + 1) * (root.hovered ? 3 : 2) * root.elevation
            x: -spread / 2
            y: spread * 0.6
            width: root.width + spread
            height: root.height + spread * 0.5
            radius: root.radius + spread / 2
            color: Theme.alpha("#000000", (Theme.dark ? 0.22 : 0.035) / (index + 1))
        }
    }

    Loader {
        anchors.fill: parent
        active: root.glow
        sourceComponent: GlowEffect { radius: root.radius; color: root.glowColor }
    }

    Rectangle {
        id: frame
        anchors.fill: parent
        radius: root.radius
        border.width: 1
        border.color: root.borderColor
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.colorTop }
            GradientStop { position: 1.0; color: root.color }
        }
        Behavior on border.color { ColorAnimation { duration: Theme.motionFast } }
    }

    // Hairline highlight along the top edge — the "crisp" card edge.
    Rectangle {
        anchors.top: parent.top
        anchors.topMargin: 1
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - root.radius * 2
        height: 1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.5; color: Theme.borderHighlight }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    Loader {
        anchors.fill: parent
        active: root.spotlight && (root.hovered || (item && item.visible))
        sourceComponent: Spotlight {
            radius: root.radius
            color: root.spotlightColor
            active: root.hovered
            position: root.pointer
        }
    }

    Loader {
        anchors.fill: parent
        active: root.trail
        sourceComponent: BorderTrail { radius: root.radius; color: root.trailColor }
    }

    Item {
        id: contentItem
        anchors.fill: parent
    }
}
