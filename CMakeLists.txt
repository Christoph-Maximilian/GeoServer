cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 14)
project(ServerGRPCTest)


find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "${PROTOBUF_INCLUDE_DIRS}")


add_subdirectory(../grpc bins EXCLUDE_FROM_ALL)
message(STATUS "Using gRPC via add_subdirectory.")

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)



set(hw_proto_srcs "${PROJECT_SOURCE_DIR}/test.pb.cc")
set(hw_proto_hdrs "${PROJECT_SOURCE_DIR}/test.pb.h")
set(hw_grpc_srcs "${PROJECT_SOURCE_DIR}/test.grpc.pb.cc" greeter_server.cpp)
set(hw_grpc_hdrs "${PROJECT_SOURCE_DIR}/test.grpc.pb.h")


# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")


add_executable(ServerGRPCTest main.cpp greeter_server.cpp ${hw_proto_srcs}  ${hw_grpc_srcs})
target_link_libraries(ServerGRPCTest   ${_GRPC_GRPCPP_UNSECURE} ${PROTOBUF_LIBRARIES})
