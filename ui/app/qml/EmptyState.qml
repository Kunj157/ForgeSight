import QtQuick

// Centered empty / zero-data state: an icon inside slowly rippling rings,
// a title, a hint and an optional action.
Column {
    id: root

    property string icon: "grid"
    property string title: ""
    property string subtitle: ""
    property string tone: "normal"
    property string actionText: ""
    signal action()

    readonly property color _tone: tone === "normal" ? Theme.accent : Theme.statusColor(tone)

    spacing: Theme.spaceMd
    width: Math.max(260, sub.implicitWidth)

    Item {
        width: 96; height: 96
        anchors.horizontalCenter: parent.horizontalCenter

        Repeater {
            model: 3
            Rectangle {
                id: ring
                anchors.centerIn: parent
                width: 44 + index * 22; height: width; radius: width / 2
                color: "transparent"
                border.width: 1
                border.color: Theme.alpha(root._tone, 0.22 - index * 0.06)
                SequentialAnimation on scale {
                    running: root.visible && !Theme.reducedMotion
                    loops: Animation.Infinite
                    PauseAnimation { duration: index * 300 }
                    NumberAnimation { from: 1; to: 1.08; duration: 1500; easing.type: Easing.InOutSine }
                    NumberAnimation { from: 1.08; to: 1; duration: 1500; easing.type: Easing.InOutSine }
                }
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: 44; height: 44; radius: 14
            color: Theme.alpha(root._tone, 0.12)
            border.width: 1
            border.color: Theme.alpha(root._tone, 0.4)
            Icon {
                anchors.centerIn: parent
                width: 20; height: 20
                name: root.icon
                color: root._tone
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.title
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontLg
        font.bold: true
        color: Theme.textPrimary
    }

    Text {
        id: sub
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.subtitle
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSm
        color: Theme.textMuted
        horizontalAlignment: Text.AlignHCenter
    }

    FxButton {
        visible: root.actionText.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.actionText
        variant: "ghost"
        onClicked: root.action()
    }
}
