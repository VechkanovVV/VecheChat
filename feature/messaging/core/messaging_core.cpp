#include "messaging_core.h"

#include <iostream>

#include "logger.h"

MessagingCore::MessagingCore(int self_id, std::shared_ptr<utils::ServiceLocator> sl,
                             std::shared_ptr<IMessagingTransport> transport)
    : self_id_(self_id), sl_(std::move(sl)), transport_(std::move(transport))
{
}

void MessagingCore::send(int leader_id, const std::string& text, std::int64_t id, std::function<void(bool ok)> onDone)
{
    auto logger = Logger::getLogger();
    if (!transport_)
    {
        if (logger) logger->error("No transport available");
        if (onDone) onDone(false);
        return;
    }

    if (logger) logger->info("Sending message to leader {}: {}", leader_id, text);

    if (self_id_ == leader_id)
    {
        broadcast(text, id, onDone);
    }
    else
    {
        transport_->sendToLeaderMessage(leader_id, text, id,
                                        [onDone, text, logger](bool ok)
                                        {
                                            if (logger)
                                            {
                                                if (ok)
                                                {
                                                    logger->info("Message delivered to leader: {}", text);
                                                }
                                                else
                                                {
                                                    logger->warn("Failed to deliver to leader: {}", text);
                                                }
                                            }
                                            if (onDone) onDone(ok);
                                        });
    }
}

void MessagingCore::broadcast(const std::string& text, std::int64_t id, std::function<void(bool ok)> onDone)
{
    auto logger = Logger::getLogger();
    if (!transport_)
    {
        if (logger) logger->error("No transport available for broadcast");
        if (onDone) onDone(false);
        return;
    }

    std::cout << "Message: " << text << std::endl;

    transport_->sendBroadcastMessage(text, id,
                                     [onDone, logger](int peerId, bool ok)
                                     {
                                         if (logger)
                                         {
                                             if (ok)
                                             {
                                                 logger->info("Message delivered to peer {}", peerId);
                                             }
                                             else
                                             {
                                                 logger->warn("Failed to deliver to peer {}", peerId);
                                             }
                                         }
                                     });

    if (onDone) onDone(true);
}

int MessagingCore::self_id() const noexcept
{
    return self_id_;
}

int MessagingCore::leader_id() const noexcept
{
    return self_id_;
}