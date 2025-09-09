cmake_minimum_required(VERSION 3.5)

set(projectName VecheChat)
project(${projectName} CXX)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

enable_testing()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/algorithm/raft
    ${CMAKE_SOURCE_DIR}/algorithm/raft/core
    ${CMAKE_SOURCE_DIR}/algorithm/raft/service
    ${CMAKE_SOURCE_DIR}/algorithm/raft/transport
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/utils/service_locator
    ${CMAKE_SOURCE_DIR}/utils/logger
    ${CMAKE_SOURCE_DIR}/utils/thread_pool
    ${CMAKE_SOURCE_DIR}/utils/timer_delegate
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/protos
    ${CMAKE_BINARY_DIR}/protos/raft
)

add_subdirectory(protos)

add_library(tp utils/thread_pool/thread_pool.cpp)
target_include_directories(tp PUBLIC utils/thread_pool)

add_library(td utils/timer_delegate/timer_delegate.cpp)
target_include_directories(td PUBLIC utils/timer_delegate)

add_library(log utils/logger/logger.cpp)
target_include_directories(log PUBLIC utils/logger)
target_link_libraries(log PUBLIC spdlog::spdlog fmt::fmt)

add_subdirectory(algorithm/raft)

add_library(RaftServer src/raft_server/raft_server.cpp)
target_include_directories(RaftServer
    PUBLIC
        src/raft_server
        ${CMAKE_BINARY_DIR}/protos
        ${CMAKE_BINARY_DIR}/protos/raft
)
target_link_libraries(RaftServer PUBLIC RaftService log)

add_executable(${projectName} src/main.cpp)
target_link_libraries(${projectName}
    PRIVATE
        RaftServer
        RaftService
        tp
        td
        log
        raft_proto
        gRPC::grpc++
        protobuf::libprotobuf
        spdlog::spdlog
        fmt::fmt
)

add_subdirectory(tests)