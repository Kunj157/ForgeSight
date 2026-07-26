include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(ingestion_tests
    test_reading_parser.cpp
    test_thread_pool.cpp
    test_event_bus.cpp
    test_db_writer.cpp
)

target_link_libraries(ingestion_tests PRIVATE
    ingestion_lib
    GTest::gtest_main
)

target_include_directories(ingestion_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/ingestion
)

gtest_discover_tests(ingestion_tests)
