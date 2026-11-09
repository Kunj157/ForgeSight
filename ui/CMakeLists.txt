# Demo mode (browser build) keeps only what the simulated data path needs:
# no SQLite offline cache, no PDF export and no network clients. See the
# FORGESIGHT_DEMO_MODE option in the root CMakeLists.txt.
if(FORGESIGHT_DEMO_MODE)
    find_package(Qt6 REQUIRED COMPONENTS Quick Charts)
else()
    find_package(Qt6 REQUIRED COMPONENTS Quick WebSockets Charts PrintSupport Network Sql)
endif()

set(CMAKE_AUTOMOC ON)

add_library(ui_lib STATIC
    src/device_model.cpp
    src/alarm_model.cpp
    src/history_model.cpp
    src/csv_export.cpp
    src/time_format.cpp
    src/series_stats.cpp
    src/rule_model.cpp
    src/chart_zoom.cpp
    src/plant_layout.cpp
    src/plant_scene_model.cpp
    src/device_filter_model.cpp
    src/display_options.cpp
    include/ui/device_model.h
    include/ui/alarm_model.h
    include/ui/history_model.h
    include/ui/csv_export.h
    include/ui/time_format.h
    include/ui/series_stats.h
    include/ui/rule_model.h
    include/ui/chart_zoom.h
    include/ui/plant_layout.h
    include/ui/plant_scene_model.h
    include/ui/device_filter_model.h
    include/ui/display_options.h
    include/ui/backend_api.h
    include/ui/live_feed.h
)

target_include_directories(ui_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(FORGESIGHT_DEMO_MODE)
    target_compile_definitions(ui_lib PUBLIC FORGESIGHT_DEMO_MODE=1)
    target_link_libraries(ui_lib PUBLIC Qt6::Quick Qt6::Charts)
else()
    target_sources(ui_lib PRIVATE
        src/ws_client.cpp
        src/api_client.cpp
        src/offline_cache.cpp
        src/pdf_export.cpp
        include/ui/ws_client.h
        include/ui/api_client.h
        include/ui/offline_cache.h
        include/ui/pdf_export.h
    )
    target_link_libraries(ui_lib PUBLIC
        Qt6::Quick
        Qt6::WebSockets
        Qt6::Charts
        Qt6::PrintSupport
        Qt6::Network
        Qt6::Sql
    )
endif()

# Browser-demo data source: a simulated plant behind the same BackendApi /
# LiveFeed interfaces the real ApiClient and WsClient implement. Linked only
# where it is used (demo mode, tests), so the desktop app does not pick up
# the demo code path.
add_library(ui_demo STATIC
    src/demo/demo_simulator.cpp
    src/demo/demo_backend.cpp
    src/demo/demo_feed.cpp
    include/ui/demo/demo_simulator.h
    include/ui/demo/demo_backend.h
    include/ui/demo/demo_feed.h
    include/ui/demo/demo_offline_stub.h
)

target_link_libraries(ui_demo PUBLIC ui_lib alarm_engine_core)
