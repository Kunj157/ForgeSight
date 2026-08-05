include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

find_package(Qt6 REQUIRED COMPONENTS Network Test)

add_executable(api_tests
    test_ws_broadcaster.cpp
    test_services.cpp
    test_api_server.cpp
)

target_link_libraries(api_tests PRIVATE
    api_lib
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    Qt6::Network
    Qt6::Test
)

gtest_discover_tests(api_tests)
