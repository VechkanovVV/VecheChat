#pragma once
#include <cstdint>
#include <functional>
#include <string>

class IMessagingTransport
{
   public:
    virtual ~IMessagingTransport() = default;

    virtual void sendToLeaderMessage(int leaderPeerId, const std::string& text, std::int64_t messageId,
                                     std::function<void(bool ok)> onReply) = 0;

    virtual void sendBroadcastMessage(const std::string& text, std::int64_t messageId,
                                      std::function<void(int peerId, bool ok)> onEachReply) = 0;

    virtual void set_rpc_timeout(int ms) = 0;
};