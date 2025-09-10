# VecheChat

A lightweight playground for building and testing Raft-based coordination and replication primitives with gRPC. It includes a minimal Raft core, a gRPC transport/service layer, unit/integration tests, and a **messaging system** that broadcasts messages through the leader.

## Features
- Raft roles: Follower, Candidate, Leader
- Elections, heartbeats (AppendEntries), and vote RPCs
- Peer membership change: RemovePeer (simplified)
- gRPC-based transport with async dispatch via a thread pool
- Protobuf API (raft.proto) and generated stubs
- CMake build with GoogleTest
- **Messaging system with broadcast functionality through leader**

## Messaging Feature
The system now includes a complete messaging layer that allows nodes to exchange messages through the current leader:

### Architecture
- **Messaging Core** (`feature/messaging/core/`) - Core messaging logic and coordination
- **Messaging Service** (`feature/messaging/service/`) - gRPC service implementation
- **Messaging Transport** (`feature/messaging/transport/`) - gRPC-based transport layer
- **Protocol Buffers** (`protos/messaging/`) - Message definitions

### How it works
1. Non-leader nodes send messages to the current leader
2. The leader broadcasts messages to all nodes in the cluster
3. All nodes display received messages in their console
4. Uses the same gRPC infrastructure as the Raft protocol

### CLI Commands
- `sendall <message>` - Send message to all nodes through the leader

## Repository layout
- `algorithm/raft/core/` — Raft state machine, timers, messages
- `algorithm/raft/transport/` — gRPC transport and interfaces
- `algorithm/raft/service/` — gRPC service implementation
- `feature/messaging/core/` — Messaging system core logic
- `feature/messaging/service/` — Messaging gRPC service
- `feature/messaging/transport/` — Messaging transport layer
- `protos/raft/` — Raft protobuf definitions
- `protos/messaging/` — Messaging protobuf definitions
- `src/raft_server/` — example server wiring
- `tests/` — unit and integration tests

## Dependencies
- CMake ≥ 3.20
- C++20 compiler
- gRPC
- Protobuf
- spdlog (logging)

## Installation
```bash
# Install build dependencies
sudo apt-get install -y \
  build-essential \
  autoconf \
  libtool \
  pkg-config \
  cmake \
  git \
  curl \
  unzip \
  libprotobuf-dev \
  protobuf-compiler \
  protobuf-compiler-grpc \
  libgrpc++-dev \
  libgrpc-dev \
  libspdlog-dev
```

## Build
```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run tests
```bash
ctest --test-dir build --output-on-failure
```

## Run example server
Build target `VecheChat`. Example for running a 3-node cluster:

```bash
# Terminal 1 - Node 1
./build/VecheChat 1 0.0.0.0:50051 2:0.0.0.0:50052 3:0.0.0.0:50053

# Terminal 2 - Node 2  
./build/VecheChat 2 0.0.0.0:50052 1:0.0.0.0:50051 3:0.0.0.0:50053

# Terminal 3 - Node 3
./build/VecheChat 3 0.0.0.0:50053 1:0.0.0.0:50051 2:0.0.0.0:50052
```

Or use the provided script to run all nodes in tmux:
```bash
chmod +x run_example.sh
./run_example.sh
```

## CLI Commands
Once the server is running, you can use the following commands:
- `stop` - Stop the server
- `get_log` - Get server logs
- `sendall <message>` - Send message to all nodes (via leader)
- `help` - Show help
- `exit` - Exit the CLI

## gRPC APIs
### Raft API (raft.v1.RaftService)
- `GetLeader(GetLeaderRequest) -> GetLeaderResponse`
- `RequestVote(RequestVoteRequest) -> RequestVoteResponse`
- `AppendEntries(AppendEntriesRequest) -> AppendEntriesResponse`
- `RemovePeer(RemovePeerRequest) -> RemovePeerResponse`

### Messaging API (messaging.v1.MessagingService)
- `SendToLeaderMessage(SendToLeaderMessageRequest) -> SendToLeaderMessageResponse`
- `SendBroadcastMessage(SendBroadcastMessageRequest) -> SendBroadcastMessageResponse`

See `protos/` directory for full schema definitions.

## Architecture
The system follows a classic Raft architecture with the addition of a messaging layer:
1. **Raft Core**: Leader election, log replication, and consistency
2. **gRPC Transport**: Network communication between nodes
3. **Messaging System**: Application-level message broadcasting
4. **Service Layer**: gRPC service implementations for both Raft and messaging

All messages are routed through the current leader, ensuring consistency and ordered delivery across the cluster.