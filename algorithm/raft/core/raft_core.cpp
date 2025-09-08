#include "raft_core.h"

#include <atomic>
#include <future>
#include <mutex>
#include <unordered_set>
#include <utility>
#include <vector>

#include "messages.h"
#include "strategies.h"

RaftCore::RaftCore(Config config, std::shared_ptr<IRaftTransport> transport) : config_(config), transport_(transport) {}

void RaftCore::start()
{
    {
        std::lock_guard<std::mutex> lock(state_mtx_);
        if (started_) return;
        started_ = true;
        role_ = Role::Follower;
        voted_for_.reset();
        leader_id_.reset();
    }
    timer_.start(std::make_unique<ElectionTimerStrategy>([this] { onElectionTimeout(); }, config_.electionMinMs,
                                                         config_.electionMaxMs));
}

void RaftCore::onElectionTimeout()
{
    std::uint64_t term_snapshot;
    std::uint64_t ll_index;
    std::uint64_t ll_term;
    {
        std::lock_guard<std::mutex> lk(state_mtx_);
        if (role_ == Role::Leader) return;
        role_ = Role::Candidate;
        current_term_++;
        voted_for_ = config_.nodeId;
        count_votes_ = 1;
        voters_.clear();
        voters_.insert(config_.nodeId);
        leader_id_.reset();
        term_snapshot = current_term_;
        ll_index = lastLogIndex();
        ll_term = lastLogTerm();
    }

    if (config_.peers.empty())
    {
        becomeLeader();
        return;
    }

    timer_.reset();

    RequestVoteRequestMsg req{term_snapshot, config_.nodeId, ll_index, ll_term};

    transport_->broadcastRequestVote(req,
                                     [this](int peerId, const RequestVoteResponseMsg& resp)
                                     {
                                         bool promote = false;
                                         bool stepDown = false;

                                         {
                                             std::lock_guard<std::mutex> lock(state_mtx_);

                                             if (resp.term > current_term_)
                                             {
                                                 stepDown = true;
                                             }

                                             else if (role_ != Role::Candidate)
                                             {
                                                 return;
                                             }

                                             else if (!resp.voteGranted)
                                             {
                                                 return;
                                             }

                                             else if (voters_.count(peerId))
                                             {
                                                 return;
                                             }
                                             else
                                             {
                                                 voters_.insert(peerId);
                                                 ++count_votes_;
                                                 const int total = static_cast<int>(config_.peers.size()) + 1;
                                                 if (count_votes_ > total / 2)
                                                 {
                                                     promote = true;
                                                 }
                                             }
                                         }

                                         if (stepDown)
                                         {
                                             updateTerm(resp.term);
                                             return;
                                         }
                                         if (promote)
                                         {
                                             becomeLeader();
                                         }
                                     });
}

void RaftCore::updateTerm(std::uint64_t new_term)
{
    std::uint64_t term_snapshot;
    {
        std::lock_guard<std::mutex> lock(state_mtx_);
        term_snapshot = current_term_;

        if (term_snapshot >= new_term) return;

        current_term_ = new_term;
        role_ = Role::Follower;
        voted_for_.reset();
        leader_id_.reset();
        count_votes_ = 0;
        voters_.clear();
    }
    timer_.changeStrategy(std::make_unique<ElectionTimerStrategy>([this] { onElectionTimeout(); },
                                                                  config_.electionMinMs, config_.electionMaxMs));
}

void RaftCore::becomeLeader()
{
    {
        std::lock_guard<std::mutex> lock(state_mtx_);
        if (role_ == Role::Leader) return;
        role_ = Role::Leader;
        leader_id_ = config_.nodeId;
    }
    timer_.changeStrategy(std::make_unique<HeartbeatTimerStrategy>([this] { sendHeartbeats(); }, config_.heartbeatMs));
    sendHeartbeats();
}

void RaftCore::sendHeartbeats()
{
    std::uint64_t term_snapshot;
    std::uint64_t ll_index;
    std::uint64_t ll_term;
    {
        std::lock_guard<std::mutex> lock(state_mtx_);
        if (role_ != Role::Leader) return;
        term_snapshot = current_term_;
        ll_index = lastLogIndex();
        ll_term = lastLogTerm();
    }

    AppendEntriesRequestMsg hb{
        term_snapshot,
        config_.nodeId,
        ll_index,
        ll_term,
        {},  // entries empty (heartbeat)
        0    // leader_commit (TODO)
    };
    transport_->broadcastAppendEntries(hb);
}

std::uint64_t RaftCore::lastLogIndex() const noexcept
{
    if (log_.empty()) return base_index_;
    return base_index_ + log_.size() - 1;
}

std::uint64_t RaftCore::lastLogTerm() const noexcept
{
    if (log_.empty()) return 0;
    return log_.back().term;
}

RequestVoteResponseMsg RaftCore::onRequestVote(const RequestVoteRequestMsg& req)
{
    bool grant{false};
    bool stepped_down{false};
    bool reject{false};
    std::uint64_t reply_term{0};
    {
        std::lock_guard<std::mutex> lock(state_mtx_);

        if (req.term < current_term_)
        {
            reply_term = current_term_;
            reject = true;
        }
        else
        {
            if (req.term > current_term_)
            {
                current_term_ = req.term;
                role_ = Role::Follower;
                voted_for_.reset();
                leader_id_.reset();
                count_votes_ = 0;
                voters_.clear();
                stepped_down = true;
            }

            reply_term = current_term_;

            if (!reject)
            {
                const auto myLastTerm = lastLogTerm();
                const auto myLastIndex = lastLogIndex();
                const bool logOk = (req.lastLogTerm > myLastTerm) ||
                                   (req.lastLogTerm == myLastTerm && req.lastLogIndex >= myLastIndex);

                if (!logOk)
                {
                    reject = true;
                }
                else if (voted_for_ && *voted_for_ != req.candidateId)
                {
                    reject = true;
                }
                else
                {
                    voted_for_ = req.candidateId;
                    grant = true;
                }
            }
        }
    }

    if (stepped_down)
    {
        timer_.changeStrategy(std::make_unique<ElectionTimerStrategy>([this] { onElectionTimeout(); },
                                                                      config_.electionMinMs, config_.electionMaxMs));
    }
    else if (grant)
    {
        timer_.reset();
    }

    return RequestVoteResponseMsg{reply_term, grant && !reject};
}

AppendEntriesResponseMsg RaftCore::onAppendEntries(const AppendEntriesRequestMsg& req)
{
    bool grant{false};
    bool need_election_strategy{false};
    std::uint64_t reply_term{0};
    std::uint64_t match_index{0};
    {
        std::lock_guard<std::mutex> lock(state_mtx_);
        if (req.term < current_term_)
        {
            reply_term = current_term_;
            return AppendEntriesResponseMsg{reply_term, false, lastLogIndex()};
        }

        Role prevRole = role_;

        if (req.term > current_term_)
        {
            current_term_ = req.term;
            role_ = Role::Follower;
            leader_id_.reset();
            voted_for_.reset();
            count_votes_ = 0;
            voters_.clear();
        }
        else if (role_ != Role::Follower)
        {
            role_ = Role::Follower;
            count_votes_ = 0;
            voters_.clear();
        }

        if (prevRole != Role::Follower)
        {
            need_election_strategy = true;
        }

        if (!leader_id_)
        {
            leader_id_ = req.leaderId;
            grant = true;
        }
        else if (*leader_id_ == req.leaderId)
        {
            grant = true;
        }
        else
        {
            grant = false;
        }

        reply_term = current_term_;
        match_index = lastLogIndex();
    }

    if (need_election_strategy)
    {
        timer_.changeStrategy(std::make_unique<ElectionTimerStrategy>([this] { onElectionTimeout(); },
                                                                      config_.electionMinMs, config_.electionMaxMs));
    }
    else if (grant)
    {
        timer_.reset();
    }

    return AppendEntriesResponseMsg{reply_term, grant, match_index};
}

Role RaftCore::role()
{
    return role_;
}

std::string RaftCore::leader_address()
{
    if (!leader_id_) return "";
    std::string add = "";
    for (const auto& p : config_.peers)
    {
        if (p.id == static_cast<uint64_t>(*leader_id_))
        {
            add = p.address;
        }
    }

    return add;
}