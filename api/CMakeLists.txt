find_package(Qt6 REQUIRED COMPONENTS HttpServer Network)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED IMPORTED_TARGET libpq)

add_library(api_lib STATIC
    src/ws_broadcaster.cpp
    src/device_service.cpp
    src/rule_service.cpp
    src/api_server.cpp
    include/api/api_server.h
)

set_target_properties(api_lib PROPERTIES AUTOMOC ON)

target_include_directories(api_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(api_lib PUBLIC
    ingestion_lib
    alarm_engine_lib
    spdlog::spdlog
    PkgConfig::LIBPQ
    Qt6::HttpServer
    Qt6::Network
    Qt6::WebSockets
)

add_executable(api main.cpp)
target_link_libraries(api PRIVATE api_lib Qt6::Core)
