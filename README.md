# VecheChat

A lightweight playground for building and testing Raft-based coordination and replication primitives with gRPC. It includes a minimal Raft core, a gRPC transport/service layer, and unit/integration tests.

## Features
- Raft roles: Follower, Candidate, Leader
- Elections, heartbeats (AppendEntries), and vote RPCs
- Peer membership change: RemovePeer (simplified)
- gRPC-based transport with async dispatch via a thread pool
- Protobuf API (raft.proto) and generated stubs
- CMake build with GoogleTest

## Repository layout
- `algorithm/raft/core/` — Raft state machine, timers, messages
- `algorithm/raft/transport/` — gRPC transport and interfaces
- `algorithm/raft/service/` — gRPC service implementation
- `protos/raft/` — protobuf definitions
- `src/raft_server/` — example server wiring
- `tests/` — unit and integration tests

## Build
Requirements: CMake ≥ 3.20, a C++20 compiler, gRPC, Protobuf.

```
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run tests
```
ctest --test-dir build --output-on-failure
```

## Run example server
Build target `RaftServer` (if enabled by your CMake presets). Example:
```
./build/RaftServer
```
Adjust command-line flags/ports as needed (see `src/raft_server/raft_server.cpp`).

## gRPC API (raft.v1.RaftService)
- `GetLeader(GetLeaderRequest) -> GetLeaderResponse`
- `RequestVote(RequestVoteRequest) -> RequestVoteResponse`
- `AppendEntries(AppendEntriesRequest) -> AppendEntriesResponse`
- `RemovePeer(RemovePeerRequest) -> RemovePeerResponse`

See `protos/raft/raft.proto` for full schema.
