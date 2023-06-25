cmake_minimum_required(VERSION 3.8)

project(aerospike-proxy-client-c C CXX)

include(cmake/common.cmake)
set(CMAKE_BUILD_TYPE Debug)

# Create the build folder
add_custom_target(build-time-make-directory ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "build")

# Proto file
get_filename_component(kvs_proto "proto/kvs.proto" ABSOLUTE)
get_filename_component(kvs_proto_path "${kvs_proto}" PATH)
get_filename_component(auth_proto "proto/auth.proto" ABSOLUTE)
get_filename_component(auth_proto_path "${auth_proto}" PATH)

# Generated sources
set(kvs_proto_srcs "build/kvs.pb.cc")
set(kvs_proto_hdrs "build/kvs.pb.h")
set(kvs_grpc_srcs "build/kvs.grpc.pb.cc")
set(kvs_grpc_hdrs "build/kvs.grpc.pb.h")
add_custom_command(
        OUTPUT "${kvs_proto_srcs}" "${kvs_proto_hdrs}" "${kvs_grpc_srcs}" "${kvs_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "build"
        --cpp_out "build"
        -I "${kvs_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${kvs_proto}"
        DEPENDS "${kvs_proto}")

# Generated sources
set(auth_proto_srcs "build/auth.pb.cc")
set(auth_proto_hdrs "build/auth.pb.h")
set(auth_grpc_srcs "build/auth.grpc.pb.cc")
set(auth_grpc_hdrs "build/auth.grpc.pb.h")
add_custom_command(
        OUTPUT "${auth_proto_srcs}" "${auth_proto_hdrs}" "${auth_grpc_srcs}" "${auth_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "build"
        --cpp_out "build"
        -I "${auth_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${auth_proto}"
        DEPENDS "${auth_proto}")


# Include generated *.pb.h files
include_directories("build" "src/include" "modules/jwt-cpp/include" "../src/include" "../modules/common/src/include" "../modules/mod-lua/src/include")

# Proxy sources
file(GLOB proxy_srcs "src/main/aerospike/*.c" "src/main/aerospike/*.cc")
file(GLOB proxy_hdrs "src/include/aerospike/*.h")

# kvs_grpc_proto
add_library(aerospike_proxy
        ${kvs_grpc_srcs}
        ${kvs_grpc_hdrs}
        ${kvs_proto_srcs}
        ${kvs_proto_hdrs}
        ${auth_grpc_srcs}
        ${auth_grpc_hdrs}
        ${auth_proto_srcs}
        ${auth_proto_hdrs}
        ${proxy_srcs}
        ${proxy_hdrs}
        )

target_link_libraries(aerospike_proxy
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})
# Put build artifacts in build directory
set_target_properties(aerospike_proxy PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "build"
        )

set(target "main")
add_executable(${target} "src/main/aerospike/${target}.cc")
target_link_libraries(${target}
        aerospike_proxy
        "/home/ashish/workspace/aerospike-client-c/target/Linux-x86_64/lib/libaerospike.a"
        absl::flags
        absl::flags_parse
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})
