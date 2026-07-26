add_executable(api
    main.cpp
)

target_include_directories(api PRIVATE ${CMAKE_SOURCE_DIR}/include/api)
