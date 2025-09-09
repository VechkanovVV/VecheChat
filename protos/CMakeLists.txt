#raft_proto
set(RAFT_PROTO_FILE
    raft/raft.proto
)

add_library(raft_proto
    ${RAFT_PROTO_FILE}
)

target_link_libraries(raft_proto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
)

target_include_directories(raft_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

protobuf_generate(
    TARGET raft_proto
    LANGUAGE cpp
)

protobuf_generate(
    TARGET raft_proto
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
)

#MessagingExchage
set(MESSAGING_PROTO_FILE
    messaging/messaging.proto
)

add_library(MessagingExchage
    ${MESSAGING_PROTO_FILE}
)

target_link_libraries(MessagingExchage
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
)

target_include_directories(MessagingExchage PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(
    TARGET MessagingExchage
    LANGUAGE cpp
)

protobuf_generate(
    TARGET MessagingExchage
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
)