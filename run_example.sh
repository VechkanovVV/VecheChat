#!/bin/bash

tmux new-session -d -s raft-nodes -n node1 'cd build && ./VecheChat 1 0.0.0.0:50051 2:0.0.0.0:50052 3:0.0.0.0:50053'

tmux split-window -v -t raft-nodes:0 'cd build && ./VecheChat 2 0.0.0.0:50052 1:0.0.0.0:50051 3:0.0.0.0:50053'
tmux split-window -v -t raft-nodes:0 'cd build && ./VecheChat 3 0.0.0.0:50053 1:0.0.0.0:50051 2:0.0.0.0:50052'

tmux select-layout -t raft-nodes:0 tiled

tmux attach-session -t raft-nodes