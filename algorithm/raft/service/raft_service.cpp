#include "raft_service.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "logger.h"
#include "messages.h"
#include "raft_core.h"

RaftServiceImpl::RaftServiceImpl(std::shared_ptr<RaftCore> core, std::shared_ptr<utils::ServiceLocator> sl,
                                 std::string node_address)
    : core_(std::move(core)), sl_(sl), node_address_(std::move(node_address))
{
}

grpc::Status RaftServiceImpl::RequestVote(grpc::ServerContext* context, const raft::v1::RequestVoteRequest* request,
                                          raft::v1::RequestVoteResponse* response)
{
    auto logger = Logger::getLogger();
    if (logger)
    {
        logger->info("Received RequestVote RPC from candidate {}", request->candidate_id());
    }

    if (!response) return grpc::Status(grpc::StatusCode::INTERNAL, "null response");
    if (!core_) return grpc::Status(grpc::StatusCode::INTERNAL, "core not initialized");

    RequestVoteRequestMsg reqm{};
    if (request)
    {
        reqm.term = static_cast<std::uint64_t>(request->term());
        reqm.candidateId = static_cast<int>(request->candidate_id());
        reqm.lastLogIndex = static_cast<std::uint64_t>(request->last_log_index());
        reqm.lastLogTerm = static_cast<std::uint64_t>(request->last_log_term());
    }

    RequestVoteResponseMsg resp = core_->onRequestVote(reqm);

    response->set_term(static_cast<int64_t>(resp.term));
    response->set_vote_granted(resp.voteGranted);

    if (logger)
    {
        logger->info("Responding to RequestVote: {} for term {}", (resp.voteGranted ? "granted" : "rejected"),
                     resp.term);
    }

    return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::AppendEntries(grpc::ServerContext* context, const raft::v1::AppendEntriesRequest* request,
                                            raft::v1::AppendEntriesResponse* response)
{
    auto logger = Logger::getLogger();
    if (logger)
    {
        logger->info("Received AppendEntries RPC from leader {}", request->leader_id());
    }

    if (!response) return grpc::Status(grpc::StatusCode::INTERNAL, "null response");
    if (!core_) return grpc::Status(grpc::StatusCode::INTERNAL, "core not initialized");

    AppendEntriesRequestMsg reqm{};
    if (request)
    {
        reqm.term = static_cast<std::uint64_t>(request->term());
        reqm.leaderId = static_cast<int>(request->leader_id());
        reqm.prevLogIndex = static_cast<std::uint64_t>(request->prev_log_index());
        reqm.prevLogTerm = static_cast<std::uint64_t>(request->prev_log_term());
        reqm.leaderCommit = static_cast<std::uint64_t>(request->leader_commit());

        reqm.entries.clear();
        for (const auto& e : request->entries())
        {
            LogEntry le{};
            le.term = static_cast<std::uint64_t>(e.term());
            const std::string& d = e.data();
            le.data.assign(d.begin(), d.end());
            reqm.entries.push_back(std::move(le));
        }
    }

    AppendEntriesResponseMsg resp = core_->onAppendEntries(reqm);

    response->set_term(static_cast<int64_t>(resp.term));
    response->set_success(resp.success);
    response->set_match_index(static_cast<int64_t>(resp.matchIndex));

    if (logger)
    {
        logger->info("Responding to AppendEntries: {} for term {}", (resp.success ? "accepted" : "rejected"),
                     resp.term);
    }

    return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::GetLeader(grpc::ServerContext* context, const raft::v1::GetLeaderRequest* request,
                                        raft::v1::GetLeaderResponse* response)
{
    auto logger = Logger::getLogger();
    if (logger)
    {
        logger->info("Received GetLeader RPC");
    }

    if (!response) return grpc::Status(grpc::StatusCode::INTERNAL, "null response");
    if (!core_) return grpc::Status(grpc::StatusCode::INTERNAL, "core not initialized");

    std::string leader_address = core_->leader_address();
    response->set_leader_address(leader_address);

    if (logger)
    {
        logger->info("Responding to GetLeader: {}", leader_address);
    }

    return grpc::Status::OK;
}

std::string RaftServiceImpl::node_address()
{
    return node_address_;
}