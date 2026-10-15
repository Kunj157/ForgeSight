#pragma once

#include <QObject>
#include <QProcessEnvironment>
#include <QString>

namespace ui {

// Startup presentation preferences read once from the environment and exposed
// to QML as constants:
//   FORGESIGHT_THEME            "light" or "dark" (default dark)
//   FORGESIGHT_REDUCED_MOTION   truthy -> no looping/decorative animation
//   FORGESIGHT_DISABLE_3D       truthy -> never try the 3D plant view
// Pure parsing, unit-tested without any UI.
class DisplayOptions : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme CONSTANT)
    Q_PROPERTY(bool reducedMotion READ reduced_motion CONSTANT)
    Q_PROPERTY(bool enable3d READ enable_3d CONSTANT)

  public:
    explicit DisplayOptions(const QProcessEnvironment& env, QObject* parent = nullptr);

    QString theme() const { return theme_; }
    bool reduced_motion() const { return reduced_motion_; }
    bool enable_3d() const { return enable_3d_; }

    static bool is_truthy(const QString& value);

  private:
    QString theme_;
    bool reduced_motion_ = false;
    bool enable_3d_ = true;
};

} // namespace ui
