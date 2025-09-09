#pragma once
#include <grpcpp/grpcpp.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "imessaging_transport.h"
#include "messaging.grpc.pb.h"
#include "peer_info.h"
#include "service_locator.h"
#include "thread_pool.h"

class MessagingGrpcTransport : public IMessagingTransport
{
   public:
    MessagingGrpcTransport(int self_id, const std::vector<PeerInfo>& peers, std::shared_ptr<utils::ServiceLocator> sl,
                           int rpc_timeout_ms = 2000);

    void sendBroadcastMessage(const std::string& text, std::int64_t id,
                              std::function<void(int peerId, bool ok)> onReply) override;

    void sendToLeaderMessage(int leaderPeerId, const std::string& text, std::int64_t id,
                             std::function<void(bool ok)> onReply) override;

    void set_rpc_timeout(int ms) override;

   private:
    struct PeerState
    {
        int id{};
        std::string address;
        std::shared_ptr<grpc::Channel> channel;
        std::unique_ptr<messaging::v1::MessagingService::Stub> stub;
    };

    int self_id_{};
    int rpc_timeout_ms_{2000};
    std::shared_ptr<utils::ServiceLocator> sl_;

    std::mutex mtx_;
    std::unordered_map<int, std::shared_ptr<PeerState>> peers_;
};