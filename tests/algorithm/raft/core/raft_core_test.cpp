#include "raft_core.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "iraft_transport.h"
#include "messages.h"
#include "peer_info.h"

struct MockTransport : IRaftTransport
{
    explicit MockTransport(std::vector<int> peers) : peers_(std::move(peers)) {}

    void broadcastRequestVote(const RequestVoteRequestMsg& req,
                              std::function<void(int, const RequestVoteResponseMsg&)> onReply) override
    {
        for (int pid : peers_)
        {
            onReply(pid, RequestVoteResponseMsg{req.term, true});
        }
    }

    void broadcastAppendEntries(const AppendEntriesRequestMsg& req,
                                std::function<void(int, const AppendEntriesResponseMsg&)> /*onReply*/) override
    {
        lastHeartbeat = req;
        heartbeatCount.fetch_add(1, std::memory_order_relaxed);
    }

    void broadcastRemovePeer(const RemovePeerRequestMsg& /*req*/) override {}

    void sendRemovePeerToPeer(int /*peerId*/, const RemovePeerRequestMsg& /*req*/) override {}

    void removePeer(std::uint64_t /*id*/) override {}

    std::vector<int> peers_;
    std::atomic<int> heartbeatCount{0};
    std::optional<AppendEntriesRequestMsg> lastHeartbeat;
};

TEST(RaftCoreTest, ElectsLeaderAndSendsHeartbeat)
{
    Config cfg;
    cfg.nodeId = 1;
    cfg.peers = {PeerInfo{2, ""}, PeerInfo{3, ""}};
    cfg.electionMinMs = 5;
    cfg.electionMaxMs = 10;
    cfg.heartbeatMs = 20;

    auto transport = std::make_shared<MockTransport>(std::vector<int>{2, 3});
    RaftCore core(cfg, transport);

    core.start();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (transport->heartbeatCount.load() == 0 && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_GE(transport->heartbeatCount.load(), 1);
    ASSERT_TRUE(transport->lastHeartbeat.has_value());
    EXPECT_EQ(transport->lastHeartbeat->leaderId, cfg.nodeId);
    EXPECT_EQ(transport->lastHeartbeat->term, 1u);
}

TEST(RaftCoreTest, TwoNodeElectionAndMultipleHeartbeats)
{
    Config cfg;
    cfg.nodeId = 1;
    cfg.peers = {PeerInfo{2, ""}};
    cfg.electionMinMs = 5;
    cfg.electionMaxMs = 10;
    cfg.heartbeatMs = 15;

    auto transport = std::make_shared<MockTransport>(std::vector<int>{2});
    RaftCore core(cfg, transport);

    core.start();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(600);
    while (transport->heartbeatCount.load() < 2 && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_GE(transport->heartbeatCount.load(), 2);
    ASSERT_TRUE(transport->lastHeartbeat.has_value());
    EXPECT_EQ(transport->lastHeartbeat->leaderId, cfg.nodeId);
}