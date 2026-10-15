include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

find_package(Qt6 REQUIRED COMPONENTS Test Quick)

find_package(Qt6 REQUIRED COMPONENTS Test Quick PrintSupport)

add_executable(ui_tests
    test_models.cpp
    test_export.cpp
    test_ws_client.cpp
    test_api_client.cpp
    test_offline_cache.cpp
    test_time_format.cpp
    test_series_stats.cpp
    test_rule_model.cpp
    test_chart_zoom.cpp
    test_device_filter_model.cpp
    test_display_options.cpp
)

target_link_libraries(ui_tests PRIVATE
    ui_lib
    GTest::gtest_main
    Qt6::Test
    Qt6::Quick
    Qt6::WebSockets
    Qt6::Network
    Qt6::PrintSupport
)

target_include_directories(ui_tests PRIVATE ${CMAKE_SOURCE_DIR}/ui/include)

gtest_discover_tests(ui_tests)

# QML component smoke tests. A separate binary because Qt Quick types need a
# QGuiApplication, while ui_tests creates a plain QCoreApplication.
add_executable(ui_qml_tests test_qml_components.cpp)
target_link_libraries(ui_qml_tests PRIVATE GTest::gtest Qt6::Quick Qt6::Qml)
target_compile_definitions(ui_qml_tests PRIVATE
    FORGESIGHT_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../ui/app/qml")
gtest_discover_tests(ui_qml_tests DISCOVERY_MODE PRE_TEST)
