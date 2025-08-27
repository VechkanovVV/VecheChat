cmake_minimum_required(VERSION 3.2)

set(projectName VecheChat)

project(${projectName})

enable_testing()

add_library(tp utils/thread_pool/thread_pool.cpp)

add_executable(${projectName}
    src/main.cpp
)

target_link_libraries(${projectName} PRIVATE tp)

add_subdirectory(tests)
