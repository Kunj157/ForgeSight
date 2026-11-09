# Pure rule model (no database, logging or ingestion dependencies) so the
# browser demo can run the same evaluator that the real engine uses.
add_library(alarm_engine_core STATIC
    src/types.cpp
    src/rule_evaluator.cpp
    src/default_rules.cpp
)
target_include_directories(alarm_engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# The engine daemon needs spdlog and libpq; the browser demo only needs the
# core above.
if(FORGESIGHT_DEMO_MODE)
    return()
endif()

find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED IMPORTED_TARGET libpq)

add_library(alarm_engine_lib STATIC
    src/alarm_store.cpp
    src/reading_cursor.cpp
)

target_include_directories(alarm_engine_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(alarm_engine_lib PUBLIC
    alarm_engine_core
    ingestion_lib
    spdlog::spdlog
    PkgConfig::LIBPQ
)

add_executable(alarm-engine main.cpp)
target_link_libraries(alarm-engine PRIVATE alarm_engine_lib)
