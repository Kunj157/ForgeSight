find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED IMPORTED_TARGET libpq)

add_library(api_lib STATIC
    src/ws_broadcaster.cpp
    src/device_service.cpp
    src/rule_service.cpp
)

target_include_directories(api_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(api_lib PUBLIC
    ingestion_lib
    alarm_engine_lib
    spdlog::spdlog
    PkgConfig::LIBPQ
)

add_library(crow_server STATIC
    src/server.cpp
)
target_include_directories(crow_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(crow_server PUBLIC
    api_lib
    Crow::Crow
)

add_executable(api main.cpp)
target_link_libraries(api PRIVATE crow_server)
