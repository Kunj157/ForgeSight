find_package(PahoMqttCpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED IMPORTED_TARGET libpq)

add_library(alarm_engine_lib STATIC
    types.cpp
    rule_evaluator.cpp
    alarm_store.cpp
)

target_include_directories(alarm_engine_lib PUBLIC ${CMAKE_SOURCE_DIR}/include/alarm-engine)
target_link_libraries(alarm_engine_lib PUBLIC
    ingestion_lib
    spdlog::spdlog
    PkgConfig::LIBPQ
)

add_executable(alarm-engine main.cpp)
target_link_libraries(alarm-engine PRIVATE alarm_engine_lib)
