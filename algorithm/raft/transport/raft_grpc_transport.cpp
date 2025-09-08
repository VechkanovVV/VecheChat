#include "raft_grpc_transport.h"

#include <grpcpp/grpcpp.h>
#include <thread_pool.h>

#include <chrono>

RaftGrpcTransport::RaftGrpcTransport(const std::vector<PeerInfo>& peers, std::shared_ptr<utils::ServiceLocator>& sl)
    : sl_(sl)
{
    for (const auto& peer : peers)
    {
        auto st = std::make_shared<PeerState>();
        st->id = static_cast<std::uint64_t>(peer.id);
        st->address = peer.address;
        st->channel = grpc::CreateChannel(peer.address, grpc::InsecureChannelCredentials());
        st->stub = raft::v1::RaftService::NewStub(st->channel);
        peers_info_.emplace(st->id, st);
    }
}

void RaftGrpcTransport::broadcastRequestVote(const RequestVoteRequestMsg& req,
                                             std::function<void(int /*peerId*/, const RequestVoteResponseMsg&)> onReply)
{
    std::vector<std::pair<std::uint64_t, std::shared_ptr<PeerState>>> peers;
    {
        std::lock_guard<std::mutex> lk(peers_mtx_);
        peers.reserve(peers_info_.size());
        for (auto& kv : peers_info_)
        {
            peers.emplace_back(kv.first, kv.second);
        }
    }

    raft::v1::RequestVoteRequest reqm;
    reqm.set_term(req.term);
    reqm.set_candidate_id(static_cast<std::uint64_t>(req.candidateId));
    reqm.set_last_log_index(req.lastLogIndex);
    reqm.set_last_log_term(req.lastLogTerm);

    for (auto& p : peers)
    {
        auto on_reply_task_ = onReply;
        auto p_id = p.first;
        auto ps = p.second;
        sl_->get<utils::ThreadPool>()->add_task(
            [on_reply_task_, p_id, ps, reqm, rpt = rpc_timeout_ms_]
            {
                if (!ps)
                {
                    RequestVoteResponseMsg fail{0, false};
                    on_reply_task_(static_cast<int>(p_id), fail);
                    return;
                }
                grpc::ClientContext ctx;
                ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(rpt));
                try
                {
                    raft::v1::RequestVoteResponse proto_resp;
                    grpc::Status status = ps->stub->RequestVote(&ctx, reqm, &proto_resp);
                    RequestVoteResponseMsg resp;
                    if (!status.ok())
                    {
                        resp.term = 0;
                        resp.voteGranted = false;
                    }
                    else
                    {
                        resp.term = proto_resp.term();
                        resp.voteGranted = proto_resp.vote_granted();
                    }
                    on_reply_task_(static_cast<int>(p_id), resp);
                }
                catch (...)
                {
                    RequestVoteResponseMsg fail{0, false};
                    on_reply_task_(static_cast<int>(p_id), fail);
                }
            });
    }
}

void RaftGrpcTransport::broadcastAppendEntries(
    const AppendEntriesRequestMsg& req, std::function<void(int /*peerId*/, const AppendEntriesResponseMsg&)> onReply)
{
    std::vector<std::pair<std::uint64_t, std::shared_ptr<PeerState>>> peers;
    {
        std::lock_guard<std::mutex> lk(peers_mtx_);
        peers.reserve(peers_info_.size());
        for (auto& kv : peers_info_)
        {
            peers.emplace_back(kv.first, kv.second);
        }
    }

    raft::v1::AppendEntriesRequest proto_req;
    proto_req.set_term(req.term);
    proto_req.set_leader_id(static_cast<std::uint64_t>(req.leaderId));
    proto_req.set_prev_log_index(req.prevLogIndex);
    proto_req.set_prev_log_term(req.prevLogTerm);
    proto_req.set_leader_commit(req.leaderCommit);

    for (auto& p : peers)
    {
        auto on_reply_task_ = onReply;
        auto p_id = p.first;
        auto ps = p.second;
        sl_->get<utils::ThreadPool>()->add_task(
            [on_reply_task_, p_id, ps, proto_req, rpt = rpc_timeout_ms_]
            {
                if (!ps)
                {
                    AppendEntriesResponseMsg fail;
                    fail.term = 0;
                    fail.success = false;
                    fail.matchIndex = 0;
                    on_reply_task_(static_cast<int>(p_id), fail);
                    return;
                }
                grpc::ClientContext ctx;
                ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(rpt));
                try
                {
                    raft::v1::AppendEntriesResponse proto_resp;
                    grpc::Status status = ps->stub->AppendEntries(&ctx, proto_req, &proto_resp);
                    AppendEntriesResponseMsg resp;
                    if (!status.ok())
                    {
                        resp.term = 0;
                        resp.success = false;
                        resp.matchIndex = 0;
                    }
                    else
                    {
                        resp.term = proto_resp.term();
                        resp.success = proto_resp.success();
                        resp.matchIndex = proto_resp.match_index();
                    }
                    on_reply_task_(static_cast<int>(p_id), resp);
                }
                catch (...)
                {
                    AppendEntriesResponseMsg fail;
                    fail.term = 0;
                    fail.success = false;
                    fail.matchIndex = 0;
                    on_reply_task_(static_cast<int>(p_id), fail);
                }
            });
    }
}

void RaftGrpcTransport::addPeer(const PeerInfo& peer, std::shared_ptr<grpc::Channel>& channel,
                                std::unique_ptr<raft::v1::RaftService::Stub>& stub)
{
    {
        std::lock_guard<std::mutex> lock(peers_mtx_);
        if (peers_info_.count(peer.id))
        {
            return;
        }
        auto ps = std::make_shared<PeerState>();
        ps->id = peer.id;
        ps->address = peer.address;
        ps->channel = std::move(channel);
        ps->stub = std::move(stub);
        peers_info_.emplace(peer.id, std::move(ps));
    }
}
void RaftGrpcTransport::removePeer(std::uint64_t id)
{
    {
        std::lock_guard<std::mutex> lock(peers_mtx_);
        if (!peers_info_.count(id))
        {
            return;
        }
        peers_info_.erase(id);
    }
}

void RaftGrpcTransport::set_rpc_timeout(int time)
{
    rpc_timeout_ms_ = time;
}
