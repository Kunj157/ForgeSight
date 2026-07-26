cmake_minimum_required(VERSION 3.20)
project(ForgeSight VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

find_package(nlohmann_json REQUIRED)

include(FetchContent)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(asio)

set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "" FORCE)
set(ASIO_FOUND TRUE CACHE BOOL "" FORCE)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.3.3
)
FetchContent_MakeAvailable(Crow)

add_subdirectory(ingestion)
add_subdirectory(alarm-engine)
add_subdirectory(api)

find_package(Qt6 COMPONENTS Quick QUIET)
if(Qt6_FOUND)
    add_subdirectory(ui/app)
else()
    message(STATUS "Qt6 not found — skipping UI build")
endif()

add_subdirectory(tests/unit)
add_subdirectory(tests/ingestion)
add_subdirectory(tests/alarm-engine)
add_subdirectory(tests/api)
