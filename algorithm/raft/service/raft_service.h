#pragma once

#include <grpcpp/grpcpp.h>

#include <memory>
#include <mutex>
#include <string>

#include "messages.h"
#include "raft.grpc.pb.h"
#include "raft.pb.h"
#include "raft_core.h"
#include "service_locator.h"

class RaftServiceImpl final : public raft::v1::RaftService::Service
{
   public:
    RaftServiceImpl(std::shared_ptr<RaftCore> core, std::shared_ptr<utils::ServiceLocator> sl,
                    std::string node_address);

    grpc::Status RequestVote(grpc::ServerContext* context, const raft::v1::RequestVoteRequest* request,
                             raft::v1::RequestVoteResponse* response) override;

    grpc::Status AppendEntries(grpc::ServerContext* context, const raft::v1::AppendEntriesRequest* request,
                               raft::v1::AppendEntriesResponse* response) override;

    grpc::Status GetLeader(grpc::ServerContext* context, const raft::v1::GetLeaderRequest* request,
                           raft::v1::GetLeaderResponse* response) override;

    grpc::Status RemovePeer(grpc::ServerContext* context, const raft::v1::RemovePeerRequest*,
                            raft::v1::RemovePeerResponse* response) override;

    std::string node_address();

   private:
    std::shared_ptr<RaftCore> core_;
    std::shared_ptr<utils::ServiceLocator> sl_;
    std::string node_address_;
};