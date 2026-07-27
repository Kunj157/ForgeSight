include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

find_package(Qt6 REQUIRED COMPONENTS Test Quick)

find_package(Qt6 REQUIRED COMPONENTS Test Quick PrintSupport)

add_executable(ui_tests
    test_models.cpp
    test_export.cpp
)

target_link_libraries(ui_tests PRIVATE
    ui_lib
    GTest::gtest_main
    Qt6::Test
    Qt6::Quick
    Qt6::PrintSupport
)

target_include_directories(ui_tests PRIVATE ${CMAKE_SOURCE_DIR}/ui/include)

gtest_discover_tests(ui_tests)
