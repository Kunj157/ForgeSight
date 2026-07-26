find_package(Qt6 REQUIRED COMPONENTS Quick)
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
)

target_link_libraries(factory-pulse PRIVATE Qt6::Quick ui_lib)
