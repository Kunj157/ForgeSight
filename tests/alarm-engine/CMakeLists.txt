include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(alarm_engine_tests
    test_types.cpp
    test_rule_evaluator.cpp
    test_alarm_store.cpp
)

target_link_libraries(alarm_engine_tests PRIVATE
    alarm_engine_lib
    ingestion_lib
    GTest::gtest_main
)

gtest_discover_tests(alarm_engine_tests)
