include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

#service_locator_test
add_executable(service_locator_test
    utils/service_locator/service_locator_test.cpp
)

target_link_libraries(service_locator_test PRIVATE GTest::gtest_main)

target_include_directories(service_locator_test
    PUBLIC
    ../utils/service_locator
)

#threadsafe_queue_test
add_executable(threadsafe_queue_test
    utils/thread_pool/threadsafe_queue_test.cpp
)

target_link_libraries(threadsafe_queue_test PRIVATE GTest::gtest_main)

target_include_directories(threadsafe_queue_test
    PUBLIC
    ../utils/thread_pool
)

#thread_pool_test
add_executable(thread_pool_test
    utils/thread_pool/thread_pool_test.cpp
)

target_link_libraries(thread_pool_test PRIVATE GTest::gtest_main tp)

target_include_directories(thread_pool_test
    PUBLIC
    ../utils/thread_pool
)

#timer_delegate_test
add_executable(timer_delegate_test
    utils/timer_delegate/timer_delegate_test.cpp
)

target_link_libraries(timer_delegate_test PRIVATE GTest::gtest_main td)

target_include_directories(timer_delegate_test
    PUBLIC
    ../utils/timer_delegate
)

#raft_strategies_test
add_executable(raft_strategies_test
    algorithm/raft/core/strategies_test.cpp
)

target_link_libraries(raft_strategies_test PRIVATE GTest::gtest_main)

target_include_directories(raft_strategies_test
    PUBLIC
    ../utils/timer_delegate
    ../algorithm/raft/core
)

#raft_core_test
add_executable(raft_core_test
    algorithm/raft/core/raft_core_test.cpp
    ../algorithm/raft/core/raft_core.cpp
    ../utils/timer_delegate/timer_delegate.cpp
)

target_link_libraries(raft_core_test PRIVATE GTest::gtest_main)

target_include_directories(raft_core_test
    PUBLIC
    ../algorithm/raft/core
    ../algorithm/raft/transport
    ../utils/timer_delegate
)

#raft_grpc_transport_test
add_executable(raft_grpc_transport_test
    algorithm/raft/transport/raft_grpc_transport_test.cpp
    ../algorithm/raft/transport/raft_grpc_transport.cpp
)

target_link_libraries(raft_grpc_transport_test PRIVATE GTest::gtest_main raft_proto td tp)

target_include_directories(raft_grpc_transport_test
    PUBLIC
    ../algorithm/raft/transport
    ../utils/service_locator
    ../utils/thread_pool
    ../algorithm/raft/core
    ${CMAKE_BINARY_DIR}/protos/raft
)

add_dependencies(raft_grpc_transport_test raft_proto tp)

#raft_service_test
add_executable(raft_service_test
    algorithm/raft/service/raft_service_test.cpp
)

target_link_libraries(raft_service_test PRIVATE
    GTest::gtest_main
    RaftService
    td
    tp
    raft_proto
)

target_include_directories(raft_service_test
    PUBLIC
    ../algorithm/raft/service
    ../utils/service_locator
    ../utils/thread_pool
    ../algorithm/raft/core
    ${CMAKE_BINARY_DIR}/protos/raft
)

add_dependencies(raft_service_test RaftService td tp raft_proto)

add_test(NAME service_locator COMMAND service_locator_test)
add_test(NAME threadsafe_queue COMMAND threadsafe_queue_test)
add_test(NAME thread_pool COMMAND thread_pool_test)
add_test(NAME timer_delegate COMMAND timer_delegate_test)
add_test(NAME raft_strategies COMMAND raft_strategies_test)
add_test(NAME raft_core COMMAND raft_core_test)
add_test(NAME raft_grpc_transport COMMAND raft_grpc_transport_test)
add_test(NAME raft_service COMMAND raft_service_test)

