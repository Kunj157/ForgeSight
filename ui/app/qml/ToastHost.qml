import QtQuick

// Bottom-right toast stack. Call show(title, message, kind) where kind is
// "ok" | "info" | "warning" | "critical". Toasts slide in, show a draining
// progress line, then slide out; hovering pauses the countdown.
Item {
    id: host

    property int timeoutMs: 4200
    property int maxToasts: 4
    property int _seq: 0

    function show(title, message, kind) {
        if (toasts.count >= maxToasts)
            toasts.remove(0)
        toasts.append({ uid: ++_seq, title: title, message: message || "", kind: kind || "info" })
    }

    function _remove(uid) {
        for (var i = 0; i < toasts.count; i++) {
            if (toasts.get(i).uid === uid) {
                toasts.remove(i)
                return
            }
        }
    }

    ListModel { id: toasts }

    Column {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spaceXl
        spacing: Theme.spaceSm

        move: Transition {
            NumberAnimation { properties: "y"; duration: Theme.motionMed; easing.type: Easing.OutCubic }
        }

        Repeater {
            model: toasts
            delegate: GlowCard {
                id: toast
                width: 340
                height: 66
                elevation: 2
                lift: false
                spotlight: false
                readonly property color tone: Theme.statusColor(model.kind)
                property real remaining: 1
                property bool leaving: false
                opacity: 0
                transform: Translate { id: shift; x: 48 }

                Component.onCompleted: enter.start()

                ParallelAnimation {
                    id: enter
                    NumberAnimation { target: toast; property: "opacity"; to: 1; duration: Theme.motionMed }
                    NumberAnimation { target: shift; property: "x"; to: 0; duration: Theme.motionSlow; easing.type: Easing.OutBack }
                }
                SequentialAnimation {
                    id: leave
                    ParallelAnimation {
                        NumberAnimation { target: toast; property: "opacity"; to: 0; duration: Theme.motionMed }
                        NumberAnimation { target: shift; property: "x"; to: 48; duration: Theme.motionMed; easing.type: Easing.InCubic }
                    }
                    ScriptAction { script: host._remove(model.uid) }
                }
                NumberAnimation on remaining {
                    from: 1; to: 0
                    duration: host.timeoutMs
                    paused: toast.hovered
                    onFinished: { toast.leaving = true; leave.start() }
                }

                Rectangle {
                    x: 10; y: 12
                    width: 3; height: parent.height - 24; radius: 1.5
                    color: toast.tone
                }

                Column {
                    x: 24; y: 13
                    width: parent.width - 60
                    spacing: 3
                    Text {
                        width: parent.width
                        text: model.title
                        elide: Text.ElideRight
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontMd
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    Text {
                        width: parent.width
                        text: model.message
                        elide: Text.ElideRight
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontXs
                        color: Theme.textSecondary
                    }
                }

                Icon {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    width: 10; height: 10
                    name: "close"
                    color: closeMouse.containsMouse ? Theme.textPrimary : Theme.textMuted
                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        anchors.margins: -6
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (!toast.leaving) { toast.leaving = true; leave.start() }
                    }
                }

                // Draining countdown line.
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 1
                    x: Theme.radiusLg
                    width: (parent.width - 2 * Theme.radiusLg) * toast.remaining
                    height: 2
                    radius: 1
                    color: Theme.alpha(toast.tone, 0.7)
                }
            }
        }
    }
}
