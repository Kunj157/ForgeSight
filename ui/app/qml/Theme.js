.pragma library

// ISA-101 / Grafana-style control-room palette.
// Saturated color is reserved for abnormal states only.

var bgApp = "#12151a"
var bgSidebar = "#0e1116"
var bgTopBar = "#161a20"
var bgPanel = "#171b21"
var bgCard = "#1c2128"
var bgCardHover = "#222830"
var bgElevated = "#252b34"
var bgInput = "#14181e"

var border = "#2c333d"
var borderStrong = "#3d4654"
var divider = "#242a33"

var textPrimary = "#e6edf3"
var textSecondary = "#9aa4b2"
var textMuted = "#6b7380"
var textInverse = "#0e1116"

var accent = "#4c8bf5"
var accentMuted = "#2a4a7a"
var accentSoft = "#1a2740"

var normal = "#8b949e"
var warning = "#d4a017"
var critical = "#e5534b"
var info = "#539bf5"
var success = "#3fb950"

var warningBg = "#2a2410"
var criticalBg = "#2a1414"
var infoBg = "#141c2a"
var successBg = "#122016"
var normalBg = "#1a1e24"

var radiusSm = 4
var radiusMd = 6
var radiusLg = 8

var spaceXs = 4
var spaceSm = 8
var spaceMd = 12
var spaceLg = 16
var spaceXl = 24

var fontXs = 11
var fontSm = 12
var fontMd = 13
var fontLg = 15
var fontXl = 18
var fontDisplay = 28

var sidebarWidth = 220
var topBarHeight = 52

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
