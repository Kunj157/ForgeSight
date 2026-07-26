find_package(spdlog REQUIRED)

add_library(api_lib STATIC
    ws_broadcaster.cpp
)

target_include_directories(api_lib PUBLIC ${CMAKE_SOURCE_DIR}/include/api)
target_link_libraries(api_lib PUBLIC
    spdlog::spdlog
)

add_executable(api main.cpp)
target_link_libraries(api PRIVATE api_lib)
