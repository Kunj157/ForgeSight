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
        qml/FxCombo.qml
        qml/PlantView25D.qml
    RESOURCES
        qml/qmldir
)

target_link_libraries(factory-pulse PRIVATE Qt6::Quick Qt6::Charts Qt6::Widgets ui_lib)

# Optional 3D plant view. Without Qt Quick 3D the Devices panel falls back to
# the 2.5D isometric view at runtime, so the app still builds and runs.
find_package(Qt6 QUIET COMPONENTS Quick3D)
if(Qt6Quick3D_FOUND)
    message(STATUS "Qt Quick 3D found — building the 3D plant view")
    target_sources(factory-pulse PRIVATE plant_instancing.cpp plant_instancing.h)
    target_link_libraries(factory-pulse PRIVATE Qt6::Quick3D)
    target_compile_definitions(factory-pulse PRIVATE FORGESIGHT_HAVE_QUICK3D=1)
    qt_target_qml_sources(factory-pulse QML_FILES qml/PlantView3D.qml qml/Splash3D.qml)
else()
    message(STATUS "Qt Quick 3D not found — 3D plant view disabled (2.5D fallback only): "
                   "${Qt6Quick3D_NOT_FOUND_MESSAGE}")
endif()
