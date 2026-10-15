import QtQuick

// Segmented control whose active pill slides between options (Motion
// Primitives' Animated Background). `model` is a list of
// { label, value, tone? }; `tone` is a status name used to colour the
// selected label (e.g. alarm severities).
Rectangle {
    id: root

    property var model: []
    property var currentValue: model.length > 0 ? model[0].value : undefined
    signal activated(var value)

    readonly property int currentIndex: {
        for (var i = 0; i < model.length; i++)
            if (model[i].value === currentValue)
                return i
        return -1
    }

    implicitHeight: 32
    implicitWidth: row.implicitWidth + 6
    radius: Theme.radiusMd
    color: Theme.bgInput
    border.width: 1
    border.color: Theme.border

    Rectangle {
        id: pill
        readonly property Item target: rep.count > 0 && root.currentIndex >= 0
                                       ? rep.itemAt(root.currentIndex) : null
        visible: target !== null
        x: target ? row.x + target.x : 0
        y: 3
        width: target ? target.width : 0
        height: root.height - 6
        radius: root.radius - 3
        color: Theme.bgElevated
        border.width: 1
        border.color: Theme.borderStrong

        Behavior on x { enabled: !Theme.reducedMotion; SpringAnimation { spring: 5; damping: 0.42; epsilon: 0.3 } }
        Behavior on width { enabled: !Theme.reducedMotion; SpringAnimation { spring: 5; damping: 0.42; epsilon: 0.3 } }

        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 8
            height: 1
            color: Theme.borderHighlight
        }
    }

    Row {
        id: row
        x: 3
        height: parent.height

        Repeater {
            id: rep
            model: root.model
            delegate: Item {
                readonly property bool selected: index === root.currentIndex
                width: label.implicitWidth + 26
                height: row.height

                Text {
                    id: label
                    anchors.centerIn: parent
                    text: modelData.label
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontXs
                    font.bold: parent.selected
                    color: !parent.selected
                           ? (mouse.containsMouse ? Theme.textPrimary : Theme.textSecondary)
                           : (modelData.tone ? Theme.statusColor(modelData.tone) : Theme.textPrimary)
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.currentValue = modelData.value
                        root.activated(modelData.value)
                    }
                }
            }
        }
    }
}
