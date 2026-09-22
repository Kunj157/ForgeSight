find_package(Qt6 REQUIRED COMPONENTS HttpServer Network)
find_package(spdlog REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED IMPORTED_TARGET libpq)
pkg_check_modules(GRPC REQUIRED IMPORTED_TARGET grpc++)
pkg_check_modules(PROTOBUF REQUIRED IMPORTED_TARGET protobuf)

find_program(GRPC_CPP_PLUGIN NAMES grpc_cpp_plugin REQUIRED)
if(NOT Protobuf_PROTOC_EXECUTABLE)
    find_program(Protobuf_PROTOC_EXECUTABLE NAMES protoc REQUIRED)
endif()

set(FORGESIGHT_PROTO_DIR ${CMAKE_SOURCE_DIR}/proto)
set(FORGESIGHT_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${FORGESIGHT_PROTO_GEN_DIR})

set(FORGESIGHT_PROTO_SRCS
    ${FORGESIGHT_PROTO_GEN_DIR}/forgesight.pb.cc
    ${FORGESIGHT_PROTO_GEN_DIR}/forgesight.grpc.pb.cc
)
set(FORGESIGHT_PROTO_HDRS
    ${FORGESIGHT_PROTO_GEN_DIR}/forgesight.pb.h
    ${FORGESIGHT_PROTO_GEN_DIR}/forgesight.grpc.pb.h
)

add_custom_command(
    OUTPUT ${FORGESIGHT_PROTO_SRCS} ${FORGESIGHT_PROTO_HDRS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --proto_path=${FORGESIGHT_PROTO_DIR}
            --cpp_out=${FORGESIGHT_PROTO_GEN_DIR}
            --grpc_out=${FORGESIGHT_PROTO_GEN_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            ${FORGESIGHT_PROTO_DIR}/forgesight.proto
    DEPENDS ${FORGESIGHT_PROTO_DIR}/forgesight.proto
    COMMENT "Generating gRPC/protobuf sources from forgesight.proto"
    VERBATIM
)

add_library(api_lib STATIC
    src/ws_broadcaster.cpp
    src/device_service.cpp
    src/rule_service.cpp
    src/api_server.cpp
    src/grpc_server.cpp
    include/api/api_server.h
    include/api/grpc_server.h
    ${FORGESIGHT_PROTO_SRCS}
)

set_target_properties(api_lib PROPERTIES AUTOMOC ON)

target_include_directories(api_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${FORGESIGHT_PROTO_GEN_DIR}
)
target_link_libraries(api_lib PUBLIC
    ingestion_lib
    alarm_engine_lib
    spdlog::spdlog
    PkgConfig::LIBPQ
    PkgConfig::GRPC
    PkgConfig::PROTOBUF
    Qt6::HttpServer
    Qt6::Network
    Qt6::WebSockets
)

add_executable(api main.cpp)
target_link_libraries(api PRIVATE api_lib Qt6::Core)
