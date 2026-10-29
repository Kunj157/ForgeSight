import QtQuick

// Rounded status badge: tinted background, coloured dot and caps label.
// With `pulse` (e.g. an unacknowledged alarm) a ring ripples out of the dot.
Rectangle {
    id: root

    property string status: "normal"
    property string text: status
    property bool pulse: false
    property bool showDot: true

    readonly property color tone: Theme.statusColor(status)

    implicitHeight: 22
    implicitWidth: row.implicitWidth + 18
    radius: height / 2
    color: Theme.statusBg(status)
    border.width: 1
    border.color: Theme.alpha(tone, status === "normal" ? 0.18 : 0.42)

    Behavior on color { ColorAnimation { duration: Theme.motionMed } }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6

        Item {
            visible: root.showDot
            width: 7; height: 7
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                id: ring
                anchors.centerIn: parent
                width: 7; height: 7; radius: 3.5
                color: "transparent"
                border.width: 1.5
                border.color: root.tone
                opacity: 0
                visible: root.pulse && !Theme.reducedMotion

                ParallelAnimation {
                    running: ring.visible && root.visible
                    loops: Animation.Infinite
                    NumberAnimation { target: ring; property: "scale"; from: 1; to: 2.8; duration: 1400; easing.type: Easing.OutCubic }
                    NumberAnimation { target: ring; property: "opacity"; from: 0.9; to: 0; duration: 1400; easing.type: Easing.OutCubic }
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 7; height: 7; radius: 3.5
                color: root.tone
            }
        }

        Text {
            text: root.text.toUpperCase()
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.bold: true
            font.letterSpacing: 0.8
            color: root.tone
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
