import QtQuick

// Odometer-style integer: each digit is a clipped 0-9 column that springs
// to its new position (Motion Primitives' Sliding Number). Digits are laid
// out right-to-left so the ones column stays put when the number grows.
Item {
    id: root

    property int value: 0
    property alias font: metrics.font
    property color color: Theme.textPrimary

    readonly property string _text: String(Math.max(0, Math.round(value)))
    readonly property real _digitW: metrics.advanceWidth("0")
    readonly property real _h: metrics.height

    implicitWidth: _text.length * _digitW
    implicitHeight: _h

    FontMetrics {
        id: metrics
        font.family: Theme.fontFamilyMono
        font.pixelSize: Theme.fontLg
        font.bold: true
    }

    Row {
        id: row
        anchors.right: parent.right
        layoutDirection: Qt.RightToLeft

        Repeater {
            model: root._text.length
            delegate: Item {
                readonly property int digit:
                    Number(root._text.charAt(root._text.length - 1 - index))
                width: root._digitW
                height: root._h
                clip: true

                Column {
                    y: -parent.digit * root._h
                    Behavior on y {
                        enabled: !Theme.reducedMotion
                        SpringAnimation { spring: 4.5; damping: 0.32; epsilon: 0.25 }
                    }
                    Repeater {
                        model: 10
                        Text {
                            width: root._digitW
                            height: root._h
                            text: index
                            font: metrics.font
                            color: root.color
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }
    }
}
