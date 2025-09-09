#pragma once

#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>
#include <vector>

#include "raft_core.h"
#include "raft_grpc_transport.h"
#include "raft_service.h"
#include "service_locator.h"
#include "thread_pool.h"

class RaftServer
{
   public:
    RaftServer(int node_id, const std::string& address, const std::vector<PeerInfo>& peers, unsigned election_min,
               unsigned election_max, unsigned heartbeat, std::shared_ptr<utils::ServiceLocator> sl);

    bool start();
    void stop();
    std::string getLogs() const;

   private:
    int node_id_;
    std::string address_;
    std::vector<PeerInfo> peers_;
    unsigned election_min_;
    unsigned election_max_;
    unsigned heartbeat_;

    std::shared_ptr<utils::ServiceLocator> sl_;
    std::shared_ptr<IRaftTransport> transport_;
    std::shared_ptr<RaftCore> core_;
    std::unique_ptr<RaftServiceImpl> service_;
    std::unique_ptr<grpc::Server> server_;
};