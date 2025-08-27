cmake_minimum_required(VERSION 3.2)

set(projectName VecheChat)

project(${projectName})

enable_testing()

add_executable(${projectName}
    src/main.cpp
)

add_subdirectory(tests/)
