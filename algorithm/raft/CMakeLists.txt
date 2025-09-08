add_library(RaftService
    core/raft_core.cpp
    transport/raft_grpc_transport.cpp
    service/raft_service.cpp
)

target_include_directories(RaftService
    PUBLIC
        ../../utils/service_locator
        ../../utils/thread_pool
        ../../utils/timer_delegate
        core
        service
        transport
        ${CMAKE_BINARY_DIR}/protos/raft
)

target_link_libraries(RaftService PUBLIC raft_proto)