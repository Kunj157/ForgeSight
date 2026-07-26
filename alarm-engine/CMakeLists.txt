find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED IMPORTED_TARGET libpq)

add_library(alarm_engine_lib STATIC
    src/types.cpp
    src/rule_evaluator.cpp
    src/alarm_store.cpp
)

target_include_directories(alarm_engine_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(alarm_engine_lib PUBLIC
    ingestion_lib
    spdlog::spdlog
    PkgConfig::LIBPQ
)

add_executable(alarm-engine main.cpp)
target_link_libraries(alarm-engine PRIVATE alarm_engine_lib)
