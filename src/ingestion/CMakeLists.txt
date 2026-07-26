add_executable(ingestion
    main.cpp
)

target_include_directories(ingestion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
