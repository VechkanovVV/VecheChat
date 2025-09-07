cmake_minimum_required(VERSION 3.2)

set(projectName VecheChat)

project(${projectName})

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

add_subdirectory(protos)

include_directories(${CMAKE_BINARY_DIR}/protos/raft)

enable_testing()

add_library(tp utils/thread_pool/thread_pool.cpp)
add_library(td utils/timer_delegate/timer_delegate.cpp)

add_executable(${projectName}
    src/main.cpp
)

target_link_libraries(${projectName} PRIVATE tp)
target_link_libraries(${projectName} PRIVATE td)

target_include_directories(${projectName}
    PUBLIC
    utils/service_locator
    utils/thread_pool
)

add_subdirectory(algorithm/raft)
add_subdirectory(tests)
