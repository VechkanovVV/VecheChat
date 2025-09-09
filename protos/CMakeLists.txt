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