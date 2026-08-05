find_package(PahoMqttCpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED IMPORTED_TARGET libpq)

if(TARGET PahoMqttCpp::paho-mqttpp3-shared)
    set(FORGESIGHT_PAHO_TARGET PahoMqttCpp::paho-mqttpp3-shared)
elseif(TARGET PahoMqttCpp::paho-mqttpp3)
    set(FORGESIGHT_PAHO_TARGET PahoMqttCpp::paho-mqttpp3)
else()
    message(FATAL_ERROR "PahoMqttCpp imported target not found")
endif()

add_library(ingestion_lib STATIC
    src/reading_parser.cpp
    src/thread_pool.cpp
    src/event_bus.cpp
    src/db_writer.cpp
)

target_include_directories(ingestion_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ingestion_lib PUBLIC
    ${FORGESIGHT_PAHO_TARGET}
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    PkgConfig::LIBPQ
)

add_executable(ingestion main.cpp)
target_link_libraries(ingestion PRIVATE ingestion_lib)
