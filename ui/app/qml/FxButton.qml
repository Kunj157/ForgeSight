import QtQuick

// Button in four variants: "primary" (accent gradient with a hover glow),
// "ghost", "danger" and "success". Presses scale down slightly; `busy`
// swaps the label for bouncing dots and blocks clicks.
Item {
    id: root

    property string text: ""
    property string icon: ""
    property string variant: "ghost"
    property bool busy: false
    property bool compact: false
    signal clicked()

    readonly property bool _primary: variant === "primary"
    readonly property color _tone: variant === "danger" ? Theme.critical
                                 : variant === "success" ? Theme.success
                                 : Theme.accent
    readonly property bool _hot: mouse.containsMouse && enabled && !busy

    implicitHeight: compact ? 28 : 34
    implicitWidth: content.implicitWidth + (compact ? 20 : 28)
    opacity: enabled ? 1 : 0.45
    scale: mouse.pressed ? 0.97 : 1
    Behavior on scale { NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic } }

    Loader {
        anchors.fill: parent
        active: root._primary && root._hot
        sourceComponent: GlowEffect { radius: Theme.radiusMd; color: Theme.accent; intensity: 0.8; breathing: false }
    }

    Rectangle {
        anchors.fill: parent
        visible: !root._primary
        radius: Theme.radiusMd
        border.width: 1
        border.color: root.variant === "ghost" ? (root._hot ? Theme.borderStrong : Theme.border)
                                               : Theme.alpha(root._tone, root._hot ? 0.9 : 0.45)
        color: root.variant === "ghost" ? (root._hot ? Theme.bgCardHover : "transparent")
             : (root._hot ? Theme.alpha(root._tone, 0.18) : Theme.alpha(root._tone, 0.08))
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }

    Rectangle {
        anchors.fill: parent
        visible: root._primary
        radius: Theme.radiusMd
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: root._hot ? Qt.lighter(Theme.accent, 1.12) : Theme.accent }
            GradientStop { position: 1.0; color: root._hot ? Qt.lighter(Theme.accent2, 1.12) : Theme.accent2 }
        }
    }

    Row {
        id: content
        anchors.centerIn: parent
        spacing: 6
        visible: !root.busy

        Icon {
            visible: root.icon.length > 0
            name: root.icon
            width: 12; height: 12
            anchors.verticalCenter: parent.verticalCenter
            color: label.color
        }
        Text {
            id: label
            text: root.text
            font.family: Theme.fontFamily
            font.pixelSize: root.compact ? Theme.fontXs : Theme.fontSm
            font.bold: true
            color: root._primary ? "#ffffff"
                 : root.variant === "ghost" ? (root._hot ? Theme.textPrimary : Theme.textSecondary)
                 : root._tone
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Row {
        anchors.centerIn: parent
        spacing: 4
        visible: root.busy
        Repeater {
            model: 3
            Rectangle {
                id: dot
                width: 5; height: 5; radius: 2.5
                color: root._primary ? "#ffffff" : Theme.textSecondary
                SequentialAnimation on opacity {
                    running: root.busy && !Theme.reducedMotion
                    loops: Animation.Infinite
                    PauseAnimation { duration: index * 140 }
                    NumberAnimation { from: 0.25; to: 1; duration: 320 }
                    NumberAnimation { from: 1; to: 0.25; duration: 320 }
                    PauseAnimation { duration: (2 - index) * 140 }
                }
            }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.enabled && !root.busy ? Qt.PointingHandCursor : Qt.ArrowCursor
        enabled: root.enabled && !root.busy
        onClicked: root.clicked()
    }
}
