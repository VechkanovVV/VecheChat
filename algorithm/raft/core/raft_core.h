#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_set>
#include <vector>

#include "iraft_transport.h"
#include "messages.h"
#include "peer_info.h"
#include "timer_delegate.h"

enum class Role
{
    Follower,
    Candidate,
    Leader
};

struct Config
{
    int nodeId;
    std::vector<PeerInfo> peers;
    unsigned int electionMinMs{150};
    unsigned int electionMaxMs{300};
    unsigned int heartbeatMs{50};
};

class RaftCore final
{
   public:
    RaftCore(Config, std::shared_ptr<IRaftTransport>);

    void start();

    Role role();

    std::string leader_address();

    RequestVoteResponseMsg onRequestVote(const RequestVoteRequestMsg&);
    AppendEntriesResponseMsg onAppendEntries(const AppendEntriesRequestMsg&);

   private:
    void becomeLeader();
    void updateTerm(std::uint64_t new_term);
    void onElectionTimeout();
    void sendHeartbeats();
    std::uint64_t lastLogIndex() const noexcept;
    std::uint64_t lastLogTerm() const noexcept;

    Config config_;
    std::shared_ptr<IRaftTransport> transport_;
    utils::TimerDelegate timer_;

    Role role_ = Role::Follower;
    std::uint64_t current_term_ = 0;
    std::optional<int> voted_for_;
    std::optional<int> leader_id_;
    std::unordered_set<int> voters_;
    std::mutex state_mtx_;
    int count_votes_{0};

    bool started_{false};

    std::vector<LogEntry> log_;
    std::uint64_t base_index_{0};
};