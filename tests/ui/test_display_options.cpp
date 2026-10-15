#include <gtest/gtest.h>

#include <QProcessEnvironment>

#include "ui/display_options.h"

using ui::DisplayOptions;

namespace {
QProcessEnvironment env_with(std::initializer_list<std::pair<const char*, const char*>> vars) {
    QProcessEnvironment env;
    for (const auto& [k, v] : vars)
        env.insert(QString::fromLatin1(k), QString::fromLatin1(v));
    return env;
}
} // namespace

TEST(DisplayOptionsTest, DefaultsToDarkWithMotionAnd3d) {
    DisplayOptions opts(QProcessEnvironment{});
    EXPECT_EQ(opts.theme(), "dark");
    EXPECT_FALSE(opts.reduced_motion());
    EXPECT_TRUE(opts.enable_3d());
}

TEST(DisplayOptionsTest, ThemeLightIsCaseInsensitive) {
    EXPECT_EQ(DisplayOptions(env_with({{"FORGESIGHT_THEME", "Light"}})).theme(), "light");
    EXPECT_EQ(DisplayOptions(env_with({{"FORGESIGHT_THEME", "dark"}})).theme(), "dark");
}

TEST(DisplayOptionsTest, UnknownThemeFallsBackToDark) {
    EXPECT_EQ(DisplayOptions(env_with({{"FORGESIGHT_THEME", "solarized"}})).theme(), "dark");
}

TEST(DisplayOptionsTest, ReducedMotionAcceptsCommonTruthyValues) {
    for (const char* v : {"1", "true", "TRUE", "yes", "on"}) {
        EXPECT_TRUE(DisplayOptions(env_with({{"FORGESIGHT_REDUCED_MOTION", v}})).reduced_motion())
            << v;
    }
    for (const char* v : {"0", "false", "", "off", "nope"}) {
        EXPECT_FALSE(DisplayOptions(env_with({{"FORGESIGHT_REDUCED_MOTION", v}})).reduced_motion())
            << v;
    }
}

TEST(DisplayOptionsTest, Disable3dTurnsOff3dView) {
    EXPECT_FALSE(DisplayOptions(env_with({{"FORGESIGHT_DISABLE_3D", "1"}})).enable_3d());
    EXPECT_TRUE(DisplayOptions(env_with({{"FORGESIGHT_DISABLE_3D", "0"}})).enable_3d());
}
