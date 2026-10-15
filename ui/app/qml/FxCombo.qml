import QtQuick
import QtQuick.Controls

// Themed ComboBox: input-style field with hover/focus border, rotating
// chevron, accent-tinted highlighted row and an elevated popup. Works with
// list models (`textRole`) and, with `editable`, as a free-text field that
// still offers the list.
ComboBox {
    id: combo

    implicitHeight: 34

    contentItem: Loader {
        sourceComponent: combo.editable ? editField : staticLabel
    }

    Component {
        id: staticLabel
        Text {
            text: combo.currentText
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            leftPadding: 12
            rightPadding: 28
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    Component {
        id: editField
        TextField {
            text: combo.editText
            onTextEdited: combo.editText = text
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            leftPadding: 12
            rightPadding: 28
            verticalAlignment: Text.AlignVCenter
            selectByMouse: true
            background: null
        }
    }

    background: Rectangle {
        color: Theme.bgInput
        radius: Theme.radiusMd
        border.width: 1
        border.color: combo.activeFocus || combo.popup.visible ? Theme.accent
                    : combo.hovered ? Theme.borderStrong : Theme.border
        Behavior on border.color { ColorAnimation { duration: Theme.motionFast } }
    }

    indicator: Icon {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 12
        width: 10; height: 10
        name: "chevron-down"
        color: Theme.textMuted
        rotation: combo.popup.visible ? 180 : 0
        Behavior on rotation { NumberAnimation { duration: Theme.motionMed } }
    }

    delegate: Component {
        ItemDelegate {
            width: combo.width
            highlighted: combo.highlightedIndex === index
            contentItem: Text {
                text: combo.textRole
                      ? (Array.isArray(combo.model) ? modelData[combo.textRole] : model[combo.textRole])
                      : modelData
                color: highlighted ? Theme.textPrimary : Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSm
            }
            background: Rectangle {
                color: highlighted ? Theme.accentSoft : "transparent"
                radius: Theme.radiusSm
            }
        }
    }

    popup.background: Rectangle {
        color: Theme.bgElevated
        radius: Theme.radiusMd
        border.color: Theme.borderStrong
    }
}
