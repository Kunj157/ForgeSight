cmake_minimum_required(VERSION 3.20)
project(ForgeSight VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

# Placed before any add_subdirectory() so the flags apply uniformly to every
# target, including FetchContent-populated googletest — mixing sanitized and
# unsanitized code in the same process is a common source of confusing
# ASan/ODR false positives.
option(FORGESIGHT_ENABLE_ASAN "Build with AddressSanitizer" OFF)
if(FORGESIGHT_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

find_package(nlohmann_json REQUIRED)

add_subdirectory(ingestion)
add_subdirectory(alarm-engine)
add_subdirectory(api)

find_package(Qt6 COMPONENTS Quick WebSockets Charts QUIET)
if(Qt6_FOUND)
    add_subdirectory(ui)
    add_subdirectory(ui/app)
else()
    message(STATUS "Qt6 not found — skipping UI build")
endif()

add_subdirectory(tests/unit)
add_subdirectory(tests/ingestion)
add_subdirectory(tests/alarm-engine)
add_subdirectory(tests/api)

if(Qt6_FOUND)
    add_subdirectory(tests/ui)
endif()
