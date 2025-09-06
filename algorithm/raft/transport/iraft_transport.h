#pragma once
#include <functional>

#include "messages.h"

struct IRaftTransport
{
    virtual ~IRaftTransport() = default;

    // Рассылает RequestVote всем пирам.
    // onReply вызывается по мере прихода ответов (peerId, ответ).
    virtual void broadcastRequestVote(const RequestVoteRequestMsg& req,
                                      std::function<void(int /*peerId*/, const RequestVoteResponseMsg&)> onReply) = 0;

    // Heartbeat / пустой AppendEntries (можно без callback на MVP).
    virtual void broadcastAppendEntries(
        const AppendEntriesRequestMsg& req,
        std::function<void(int /*peerId*/, const AppendEntriesResponseMsg&)> onReply = {}) = 0;
};