import QtQuick

// A number that eases to each new value instead of jumping (Motion
// Primitives' Animated Number), with an optional brief tint toward
// `upColor` / `downColor` so a live change is noticeable at a glance.
Text {
    id: root

    property real value: 0
    property int decimals: 1
    property string prefix: ""
    property string suffix: ""
    property color baseColor: Theme.textPrimary
    property bool flashOnChange: false
    property color upColor: Theme.success
    property color downColor: Theme.info
    property int duration: 520

    property real displayed: value
    property real _flash: 0
    property color _flashColor: upColor
    property real _last: value

    Behavior on displayed {
        enabled: !Theme.reducedMotion
        NumberAnimation { duration: root.duration; easing.type: Easing.OutCubic }
    }

    onValueChanged: {
        if (flashOnChange && !Theme.reducedMotion && isFinite(_last) && value !== _last) {
            _flashColor = value > _last ? upColor : downColor
            flashAnim.restart()
        }
        _last = value
    }

    NumberAnimation {
        id: flashAnim
        target: root
        property: "_flash"
        from: 1; to: 0
        duration: 900
        easing.type: Easing.OutQuad
    }

    text: isFinite(displayed) ? prefix + displayed.toFixed(decimals) + suffix : "—"
    color: _flash > 0 ? Qt.tint(baseColor, Theme.alpha(_flashColor, 0.65 * _flash)) : baseColor
    font.family: Theme.fontFamilyMono
}
