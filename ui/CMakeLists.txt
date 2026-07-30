find_package(Qt6 REQUIRED COMPONENTS Quick WebSockets Charts PrintSupport Network)

set(CMAKE_AUTOMOC ON)

add_library(ui_lib STATIC
    src/device_model.cpp
    src/alarm_model.cpp
    src/ws_client.cpp
    src/history_model.cpp
    src/csv_export.cpp
    src/pdf_export.cpp
    src/api_client.cpp
    include/ui/device_model.h
    include/ui/alarm_model.h
    include/ui/ws_client.h
    include/ui/history_model.h
    include/ui/csv_export.h
    include/ui/pdf_export.h
    include/ui/api_client.h
)

target_include_directories(ui_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ui_lib PUBLIC
    Qt6::Quick
    Qt6::WebSockets
    Qt6::Charts
    Qt6::PrintSupport
    Qt6::Network
)
