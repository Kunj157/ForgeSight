// Instantiates the dashboard's reusable QML components straight from the
// source tree, so a typo, a missing import or a bad binding in a primitive
// fails CI instead of surfacing as a blank panel at runtime.
#include <gtest/gtest.h>

#include <QColor>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QScopedPointer>
#include <QUrl>
#include <QVariant>

#include <memory>

namespace {

const QString kQmlDir = QStringLiteral(FORGESIGHT_QML_DIR);

class QmlComponentsTest : public ::testing::Test {
  protected:
    // Compiles `body` as if it were a file living next to the real components,
    // so the implicit directory import (and its qmldir singleton) applies.
    QObject* make(const QByteArray& body) {
        QQmlComponent component(&engine_);
        component.setData(body, QUrl::fromLocalFile(kQmlDir + "/__qml_test__.qml"));
        QObject* obj = component.create();
        last_errors_ = component.errorString();
        if (obj)
            obj->setParent(&engine_);
        return obj;
    }

    QObject* load_file(const QString& name) {
        QQmlComponent component(&engine_, QUrl::fromLocalFile(kQmlDir + "/" + name));
        QObject* obj = component.create();
        last_errors_ = component.errorString();
        if (obj)
            obj->setParent(&engine_);
        return obj;
    }

    QQmlEngine engine_;
    QString last_errors_;
};

TEST_F(QmlComponentsTest, ThemeSingletonResolvesAndSwitchesMode) {
    QObject* obj = make("import QtQuick\n"
                        "Item {\n"
                        "  property color app: Theme.bgApp\n"
                        "  property int med: Theme.motionMed\n"
                        "  function light() { Theme.dark = false }\n"
                        "  function still() { Theme.reducedMotion = true }\n"
                        "}\n");
    ASSERT_NE(obj, nullptr) << last_errors_.toStdString();

    const QColor dark_bg = obj->property("app").value<QColor>();
    EXPECT_GT(obj->property("med").toInt(), 0);

    QMetaObject::invokeMethod(obj, "light");
    EXPECT_NE(obj->property("app").value<QColor>(), dark_bg);
    EXPECT_GT(obj->property("app").value<QColor>().lightness(), dark_bg.lightness());

    QMetaObject::invokeMethod(obj, "still");
    EXPECT_EQ(obj->property("med").toInt(), 0);
}

TEST_F(QmlComponentsTest, ThemeStatusHelpersCoverEveryState) {
    QObject* obj =
        make("import QtQuick\n"
             "Item {\n"
             "  property var states: ['critical','warning','info','stale','ok','normal']\n"
             "  property int distinct: {\n"
             "    var seen = {}\n"
             "    for (var i = 0; i < states.length; i++)\n"
             "      seen[String(Theme.statusColor(states[i]))] = true\n"
             "    return Object.keys(seen).length\n"
             "  }\n"
             "  property real halfAlpha: Theme.alpha(Theme.critical, 0.5).a\n"
             "}\n");
    ASSERT_NE(obj, nullptr) << last_errors_.toStdString();
    EXPECT_EQ(obj->property("distinct").toInt(), 6);
    EXPECT_NEAR(obj->property("halfAlpha").toDouble(), 0.5, 0.01);
}

TEST_F(QmlComponentsTest, BorderTrailStopsUnderReducedMotion) {
    QObject* obj = make("import QtQuick\n"
                        "BorderTrail { width: 200; height: 100 }\n");
    ASSERT_NE(obj, nullptr) << last_errors_.toStdString();
    EXPECT_GT(obj->property("perimeter").toDouble(), 500.0);
    EXPECT_TRUE(obj->property("_active").toBool());
    QObject* toggler =
        make("import QtQuick\nItem { Component.onCompleted: Theme.reducedMotion = true }\n");
    ASSERT_NE(toggler, nullptr);
    EXPECT_FALSE(obj->property("_active").toBool());
}

class ComponentFileTest : public QmlComponentsTest,
                          public ::testing::WithParamInterface<const char*> {};

TEST_P(ComponentFileTest, InstantiatesWithoutErrors) {
    QObject* obj = load_file(QString::fromLatin1(GetParam()));
    EXPECT_NE(obj, nullptr) << last_errors_.toStdString();
}

INSTANTIATE_TEST_SUITE_P(Primitives, ComponentFileTest,
                         ::testing::Values("Icon.qml", "ShadowCard.qml", "GlowCard.qml",
                                           "GlowEffect.qml", "Spotlight.qml", "BorderTrail.qml"));

} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
