find_package(PahoMqttCpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED IMPORTED_TARGET libpq)

add_library(ingestion_lib STATIC
    reading_parser.cpp
    thread_pool.cpp
    event_bus.cpp
    db_writer.cpp
)

target_include_directories(ingestion_lib PUBLIC ${CMAKE_SOURCE_DIR}/include/ingestion)
target_link_libraries(ingestion_lib PUBLIC
    PahoMqttCpp::paho-mqttpp3-shared
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    PkgConfig::LIBPQ
)

add_executable(ingestion main.cpp)
target_link_libraries(ingestion PRIVATE ingestion_lib)
