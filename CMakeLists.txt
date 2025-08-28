cmake_minimum_required(VERSION 3.2)

set(projectName VecheChat)

project(${projectName})

enable_testing()

add_library(tp utils/thread_pool/thread_pool.cpp)
add_library(td utils/timer_delegate/timer_delegate.cpp)

add_executable(${projectName}
    src/main.cpp
)

target_link_libraries(${projectName} PRIVATE tp)
target_link_libraries(${projectName} PRIVATE td)

add_subdirectory(tests)
