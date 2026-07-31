find_package(Qt6 REQUIRED COMPONENTS Quick Charts Widgets)

qt_standard_project_setup()

qt_add_executable(factory-pulse
    main.cpp
)

qt_add_qml_module(factory-pulse
    URI app
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/DeviceTreePanel.qml
        qml/AlarmPanel.qml
        qml/HistoryPanel.qml
        qml/Icon.qml
        qml/ShadowCard.qml
        qml/Theme.js
)

target_link_libraries(factory-pulse PRIVATE Qt6::Quick Qt6::Charts Qt6::Widgets ui_lib)
