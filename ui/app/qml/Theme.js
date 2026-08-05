.pragma library

// Modern control-room dashboard palette — inspired by Grafana Cloud /
// Linear / Vercel dark themes. ISA-101 principle retained: saturated
// color is reserved for abnormal states, not decoration.

var bgApp = "#0a0d12"
var bgSidebar = "#0b0e13"
var bgSidebarBottom = "#090b0f"
var bgTopBar = "#0d1016"
var bgPanel = "#0d1016"
var bgCard = "#12161d"
var bgCardHover = "#161b24"
var bgElevated = "#1a212b"
var bgInput = "#0f131a"
var bgTooltip = "#1c232f"

var border = "#1f2530"
var borderStrong = "#323b49"
var divider = "#1a2029"

var textPrimary = "#eef2f7"
var textSecondary = "#8f9bac"
var textMuted = "#5a6474"
var textInverse = "#0a0d12"

// Two-tone accent used for gradients (buttons, active nav, glows).
var accent = "#5b8cff"
var accent2 = "#8b6bff"
var accentMuted = "#2a4a8a"
var accentSoft = "#151d33"
var accentGlow = "#5b8cff"

var normal = "#7c8797"
var warning = "#e0a940"
var critical = "#f0564c"
var info = "#5b9dff"
var success = "#3ecf8e"

var warningBg = "#2a2313"
var criticalBg = "#2d1616"
var infoBg = "#141f30"
var successBg = "#0f2620"
var normalBg = "#161b22"

var radiusSm = 6
var radiusMd = 10
var radiusLg = 14
var radiusXl = 18

var spaceXs = 4
var spaceSm = 8
var spaceMd = 12
var spaceLg = 16
var spaceXl = 24

var fontXs = 11
var fontSm = 12
var fontMd = 13
var fontLg = 15
var fontXl = 19
var fontDisplay = 30

var fontFamily = "Ubuntu"
var fontFamilyMono = "Ubuntu Mono"

var sidebarWidth = 232
var topBarHeight = 60

var motionFast = 110
var motionMed = 180
var motionSlow = 320
// Numeric QEasingCurve::Type values — Theme.js is a .pragma library (plain
// JS), so it has no access to the QML "Easing" enum global.
var easeOut = 6      // Easing.OutCubic
var easeInOut = 7     // Easing.InOutCubic

function statusColor(status) {
    if (status === "critical") return critical
    if (status === "warning") return warning
    if (status === "info") return info
    return normal
}

function statusBg(status) {
    if (status === "critical") return criticalBg
    if (status === "warning") return warningBg
    if (status === "info") return infoBg
    return normalBg
}

function alpha(hex, a) {
    // hex like "#5b8cff" -> rgba string usable as Qt color "#RRGGBBAA" isn't
    // supported the same way everywhere, so build via Qt.rgba through callers.
    return hex + Math.round(a * 255).toString(16).padStart(2, "0")
}
