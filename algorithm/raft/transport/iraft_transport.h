#pragma once
#include <functional>

#include "messages.h"

struct IRaftTransport
{
    virtual ~IRaftTransport() = default;

    virtual void broadcastRequestVote(const RequestVoteRequestMsg& req,
                                      std::function<void(int /*peerId*/, const RequestVoteResponseMsg&)> onReply) = 0;

    virtual void broadcastAppendEntries(
        const AppendEntriesRequestMsg& req,
        std::function<void(int /*peerId*/, const AppendEntriesResponseMsg&)> onReply = {}) = 0;

    virtual void broadcastRemovePeer(const RemovePeerRequestMsg&) = 0;

    virtual void removePeer(std::uint64_t id) = 0;

    virtual void sendRemovePeerToPeer(int peerId, const RemovePeerRequestMsg& req) = 0;
};