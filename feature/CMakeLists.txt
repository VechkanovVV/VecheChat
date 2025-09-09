add_library(Messaging
    messaging/core/messaging_core.cpp
    messaging/transport/messaging_grpc_transport.cpp
    messaging/service/messaging_service.cpp
)

target_include_directories(Messaging
    PUBLIC
        ../../utils/service_locator
        ../../utils/thread_pool
        ../../utils/logger
        messaging/core
        messaging/service
        messaging/transport
        ${CMAKE_BINARY_DIR}/protos/messaging
)

target_link_libraries(Messaging PUBLIC MessagingExchage log)