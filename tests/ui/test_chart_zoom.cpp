#include <gtest/gtest.h>

#include <QCoreApplication>

#include "ui/chart_zoom.h"

namespace {

double mapMin(const QVariantMap& m) {
    return m.value("min").toDouble();
}
double mapMax(const QVariantMap& m) {
    return m.value("max").toDouble();
}

} // namespace

class ChartZoomTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }
    ui::ChartZoom z;
};

// Zooming in around the centre shrinks the span symmetrically.
TEST_F(ChartZoomTest, ZoomInAroundCentre) {
    const auto r = z.zoom(0.0, 100.0, 0.5, 0.5);
    EXPECT_DOUBLE_EQ(mapMin(r), 25.0);
    EXPECT_DOUBLE_EQ(mapMax(r), 75.0);
}

// The value under the focal point must stay put after zooming, so the user
// zooms toward the cursor rather than the centre.
TEST_F(ChartZoomTest, ZoomKeepsFocalPointStationary) {
    const double focal = 0.25;
    const double focalValue = 0.0 + focal * (100.0 - 0.0); // 25
    const auto r = z.zoom(0.0, 100.0, focal, 0.5);
    // newSpan = 50; newMin = 25 - 0.25*50 = 12.5; newMax = 25 + 0.75*50 = 62.5
    EXPECT_DOUBLE_EQ(mapMin(r), 12.5);
    EXPECT_DOUBLE_EQ(mapMax(r), 62.5);
    // The focal value maps back to the same fraction of the new range.
    const double frac = (focalValue - mapMin(r)) / (mapMax(r) - mapMin(r));
    EXPECT_NEAR(frac, focal, 1e-9);
}

// Zooming out (scale > 1) grows the span.
TEST_F(ChartZoomTest, ZoomOutGrowsSpan) {
    const auto r = z.zoom(0.0, 100.0, 0.5, 2.0);
    EXPECT_DOUBLE_EQ(mapMin(r), -50.0);
    EXPECT_DOUBLE_EQ(mapMax(r), 150.0);
}

// Focal fractions outside [0,1] are clamped rather than producing a skewed
// range (the pointer can sit just outside the plot area).
TEST_F(ChartZoomTest, FocalFractionIsClamped) {
    const auto lo = z.zoom(0.0, 100.0, -1.0, 0.5); // clamps to 0.0
    EXPECT_DOUBLE_EQ(mapMin(lo), 0.0);
    EXPECT_DOUBLE_EQ(mapMax(lo), 50.0);
    const auto hi = z.zoom(0.0, 100.0, 5.0, 0.5); // clamps to 1.0
    EXPECT_DOUBLE_EQ(mapMin(hi), 50.0);
    EXPECT_DOUBLE_EQ(mapMax(hi), 100.0);
}

// Degenerate/invalid inputs are returned unchanged instead of producing NaN or
// an inverted range.
TEST_F(ChartZoomTest, InvalidInputsReturnedUnchanged) {
    const auto empty = z.zoom(50.0, 50.0, 0.5, 0.5); // zero span
    EXPECT_DOUBLE_EQ(mapMin(empty), 50.0);
    EXPECT_DOUBLE_EQ(mapMax(empty), 50.0);
    const auto inverted = z.zoom(100.0, 0.0, 0.5, 0.5); // max < min
    EXPECT_DOUBLE_EQ(mapMin(inverted), 100.0);
    EXPECT_DOUBLE_EQ(mapMax(inverted), 0.0);
    const auto zeroScale = z.zoom(0.0, 100.0, 0.5, 0.0); // non-positive scale
    EXPECT_DOUBLE_EQ(mapMin(zeroScale), 0.0);
    EXPECT_DOUBLE_EQ(mapMax(zeroScale), 100.0);
}

// Panning shifts both ends by the same fraction of the span; the span is
// preserved.
TEST_F(ChartZoomTest, PanShiftsRangeByFraction) {
    const auto right = z.pan(0.0, 100.0, 0.1);
    EXPECT_DOUBLE_EQ(mapMin(right), 10.0);
    EXPECT_DOUBLE_EQ(mapMax(right), 110.0);

    const auto left = z.pan(0.0, 100.0, -0.25);
    EXPECT_DOUBLE_EQ(mapMin(left), -25.0);
    EXPECT_DOUBLE_EQ(mapMax(left), 75.0);
}

TEST_F(ChartZoomTest, PanInvalidSpanUnchanged) {
    const auto r = z.pan(50.0, 50.0, 0.5);
    EXPECT_DOUBLE_EQ(mapMin(r), 50.0);
    EXPECT_DOUBLE_EQ(mapMax(r), 50.0);
}
