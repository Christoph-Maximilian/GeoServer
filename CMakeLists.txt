cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 14)
project(ServerGRPCTest)

# S2 dependency
if (NOT TARGET s2)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../s2geometry/third_party/cmake")
    add_subdirectory(../s2geometry s2geometry)
endif()

# Protobuf
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "${PROTOBUF_INCLUDE_DIRS}")

# gRPC
add_subdirectory(../grpc bins EXCLUDE_FROM_ALL)
message(STATUS "Using gRPC via add_subdirectory.")

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

# Set the name of the used proto file that defines the grpc service
set(protobuf_service_definition geoanalysis)

set(hw_proto_srcs "${PROJECT_SOURCE_DIR}/${protobuf_service_definition}.pb.cc")
set(hw_proto_hdrs "${PROJECT_SOURCE_DIR}/${protobuf_service_definition}.pb.h")
set(hw_grpc_srcs "${PROJECT_SOURCE_DIR}/${protobuf_service_definition}.grpc.pb.cc" geo_analysis_server.cpp)
set(hw_grpc_hdrs "${PROJECT_SOURCE_DIR}/${protobuf_service_definition}.grpc.pb.h")


# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_executable(ServerGRPCTest geo_analysis_server.cpp ${hw_proto_srcs}  ${hw_grpc_srcs} data.cpp data.h)

target_link_libraries(ServerGRPCTest ${_GRPC_GRPCPP_UNSECURE} ${PROTOBUF_LIBRARIES} s2)
