#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "iraft_transport.h"
#include "messages.h"
#include "peer_info.h"
#include "raft.grpc.pb.h"
#include "raft.pb.h"
#include "service_locator.h"

class RaftGrpcTransport : public IRaftTransport
{
   public:
    explicit RaftGrpcTransport(const std::vector<PeerInfo>&, std::shared_ptr<utils::ServiceLocator>&);
    ~RaftGrpcTransport() override = default;

    void broadcastRequestVote(const RequestVoteRequestMsg& req,
                              std::function<void(int /*peerId*/, const RequestVoteResponseMsg&)> onReply) override;

    void broadcastAppendEntries(const AppendEntriesRequestMsg& req,
                                std::function<void(int /*peerId*/, const AppendEntriesResponseMsg&)> onReply) override;

    void addPeer(const PeerInfo&, std::shared_ptr<grpc::Channel>&, std::unique_ptr<raft::v1::RaftService::Stub>&);
    void removePeer(std::uint64_t);

   private:
    struct PeerState
    {
        std::uint64_t id{};
        std::string address;
        std::shared_ptr<grpc::Channel> channel;
        std::unique_ptr<raft::v1::RaftService::Stub> stub;
    };

    std::shared_ptr<utils::ServiceLocator> sl_;

    std::mutex peers_mtx_;
    std::unordered_map<std::uint64_t, std::shared_ptr<PeerState>> peers_info_;
    int rpc_timeout_ms_ = 200;
};