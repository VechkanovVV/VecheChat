add_library(RaftService
    core/raft_core.cpp
)

target_include_directories(RaftService
    PUBLIC
        ../../utils/service_locator
        ../../utils/thread_pool
        ../../utils/timer_delegate
        core
        service
        transport
)

target_link_libraries(RaftService
    PUBLIC
        raft_proto
)