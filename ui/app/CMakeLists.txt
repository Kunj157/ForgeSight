find_package(Qt6 REQUIRED COMPONENTS Quick Charts Widgets)

qt_standard_project_setup()

qt_add_executable(factory-pulse
    main.cpp
)

# Theme.qml is a singleton. The hand-written qml/qmldir declares it for the
# implicit directory import that every panel uses (the files are loaded from
# qrc:/app/qml/, not as module "app" types), so it is shipped as a resource
# next to them.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(factory-pulse
    URI app
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/DeviceTreePanel.qml
        qml/AlarmPanel.qml
        qml/HistoryPanel.qml
        qml/RuleEditorPanel.qml
        qml/Icon.qml
        qml/ShadowCard.qml
        qml/Theme.qml
        qml/GlowCard.qml
        qml/GlowEffect.qml
        qml/Spotlight.qml
        qml/BorderTrail.qml
        qml/AnimatedNumber.qml
        qml/SlidingNumber.qml
        qml/StatusPill.qml
        qml/ShimmerText.qml
        qml/Skeleton.qml
        qml/SegmentedControl.qml
        qml/FxButton.qml
        qml/EmptyState.qml
        qml/ToastHost.qml
        qml/Sidebar.qml
        qml/TopBar.qml
        qml/ConnectionCard.qml
        qml/DeviceCard.qml
        qml/Sparkline.qml
    RESOURCES
        qml/qmldir
)

target_link_libraries(factory-pulse PRIVATE Qt6::Quick Qt6::Charts Qt6::Widgets ui_lib)
