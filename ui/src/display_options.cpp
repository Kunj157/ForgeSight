#include "ui/display_options.h"

namespace ui {

DisplayOptions::DisplayOptions(const QProcessEnvironment& env, QObject* parent) : QObject(parent) {
    const QString theme = env.value(QStringLiteral("FORGESIGHT_THEME")).trimmed().toLower();
    theme_ = theme == QLatin1String("light") ? QStringLiteral("light") : QStringLiteral("dark");
    reduced_motion_ = is_truthy(env.value(QStringLiteral("FORGESIGHT_REDUCED_MOTION")));
    enable_3d_ = !is_truthy(env.value(QStringLiteral("FORGESIGHT_DISABLE_3D")));
}

bool DisplayOptions::is_truthy(const QString& value) {
    const QString v = value.trimmed().toLower();
    return v == QLatin1String("1") || v == QLatin1String("true") || v == QLatin1String("yes") ||
           v == QLatin1String("on");
}

} // namespace ui
