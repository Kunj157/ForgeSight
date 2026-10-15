pragma Singleton
import QtQuick

// Design tokens for the dashboard. A singleton QtObject rather than a JS
// library so every binding re-evaluates when `dark` flips — the whole UI
// re-themes live. ISA-101 principle retained: saturated colour is reserved
// for abnormal states (critical / warning / stale), never decoration.
QtObject {
    id: t

    // —— Mode ——
    property bool dark: true
    // Collapses every motion token to 0 and lets components skip looping
    // effects (pulses, trails, shimmer). Driven by FORGESIGHT_REDUCED_MOTION.
    property bool reducedMotion: false

    // —— Surfaces ——
    readonly property color bgApp: dark ? "#06080c" : "#f3f5f8"
    readonly property color bgSidebar: dark ? "#090c11" : "#fbfcfd"
    readonly property color bgSidebarBottom: dark ? "#07090d" : "#f5f7fa"
    readonly property color bgTopBar: dark ? "#080b10" : "#ffffff"
    readonly property color bgPanel: dark ? "#0a0d12" : "#f3f5f8"
    readonly property color bgCard: dark ? "#0f131a" : "#ffffff"
    readonly property color bgCardTop: dark ? "#131821" : "#ffffff"
    readonly property color bgCardHover: dark ? "#141a23" : "#f8f9fb"
    readonly property color bgElevated: dark ? "#181e29" : "#eef1f5"
    readonly property color bgInput: dark ? "#0b0f15" : "#ffffff"
    readonly property color bgTooltip: dark ? "#1b2230" : "#111827"
    readonly property color overlay: dark ? "#b3000000" : "#59111827"

    readonly property color border: dark ? "#1b212b" : "#e3e7ed"
    readonly property color borderStrong: dark ? "#2b3341" : "#cbd2dc"
    readonly property color borderHighlight: dark ? "#ffffff14" : "#ffffffcc"
    readonly property color divider: dark ? "#151a22" : "#eceff3"

    // —— Text ——
    readonly property color textPrimary: dark ? "#edf1f7" : "#0e1520"
    readonly property color textSecondary: dark ? "#9ba7b9" : "#465264"
    readonly property color textMuted: dark ? "#606c7e" : "#7a8597"
    readonly property color textInverse: dark ? "#06080c" : "#ffffff"

    // —— Accent (two-tone, for gradients, focus and selection) ——
    readonly property color accent: dark ? "#5b8cff" : "#2f66f0"
    readonly property color accent2: dark ? "#9a7bff" : "#6d4ae6"
    readonly property color accentMuted: dark ? "#2a4a8a" : "#b9cbfb"
    readonly property color accentSoft: dark ? "#131c33" : "#eaf0ff"
    readonly property color accentGlow: accent

    // —— Status (ok / warn / critical / stale / info) ——
    readonly property color normal: dark ? "#7c8797" : "#6b7686"
    readonly property color success: dark ? "#3ecf8e" : "#0f9f6e"
    readonly property color warning: dark ? "#f2b545" : "#b7650b"
    readonly property color critical: dark ? "#ff5d55" : "#d92d2d"
    readonly property color info: dark ? "#5ba4ff" : "#2563eb"
    readonly property color stale: dark ? "#a08cd6" : "#7559c2"

    readonly property color successBg: dark ? "#0d241c" : "#e6f6ef"
    readonly property color warningBg: dark ? "#2a2111" : "#fdf3e1"
    readonly property color criticalBg: dark ? "#2c1214" : "#fdecec"
    readonly property color infoBg: dark ? "#111d30" : "#e8f0fe"
    readonly property color staleBg: dark ? "#1c1829" : "#f1edfb"
    readonly property color normalBg: dark ? "#151a22" : "#f0f2f5"

    readonly property color shadow: dark ? "#8c000000" : "#1f1b2433"

    // —— Shape & spacing ——
    readonly property int radiusSm: 6
    readonly property int radiusMd: 10
    readonly property int radiusLg: 14
    readonly property int radiusXl: 18

    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 12
    readonly property int spaceLg: 16
    readonly property int spaceXl: 24
    readonly property int space2xl: 32

    // —— Type scale ——
    readonly property int fontXs: 11
    readonly property int fontSm: 12
    readonly property int fontMd: 13
    readonly property int fontLg: 15
    readonly property int fontXl: 19
    readonly property int fontDisplay: 30
    readonly property int fontHero: 40
    readonly property real trackingCaps: 1.2

    readonly property string fontFamily: "Ubuntu"
    readonly property string fontFamilyMono: "Ubuntu Mono"

    readonly property int sidebarWidth: 232
    readonly property int topBarHeight: 64

    // —— Motion ——
    readonly property int motionFast: reducedMotion ? 0 : 120
    readonly property int motionMed: reducedMotion ? 0 : 200
    readonly property int motionSlow: reducedMotion ? 0 : 360
    readonly property int motionLoop: 1600
    readonly property int easeOut: Easing.OutCubic
    readonly property int easeInOut: Easing.InOutCubic
    readonly property int easeSpring: Easing.OutBack

    function statusColor(status) {
        if (status === "critical") return critical
        if (status === "warning") return warning
        if (status === "info") return info
        if (status === "stale") return stale
        if (status === "ok" || status === "success") return success
        return normal
    }

    function statusBg(status) {
        if (status === "critical") return criticalBg
        if (status === "warning") return warningBg
        if (status === "info") return infoBg
        if (status === "stale") return staleBg
        if (status === "ok" || status === "success") return successBg
        return normalBg
    }

    // Same colour at a new alpha; Qt.lighter(c, 1) normalises any colour
    // spec (string or color) into an object exposing r/g/b.
    function alpha(c, a) {
        var q = Qt.lighter(c, 1.0)
        return Qt.rgba(q.r, q.g, q.b, a)
    }

    function toggle() { dark = !dark }
}
