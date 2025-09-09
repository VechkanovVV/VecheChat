#include "raft_grpc_transport.h"

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "raft.grpc.pb.h"
#include "raft.pb.h"
#include "service_locator.h"
#include "thread_pool.h"

using namespace std::chrono_literals;

class FakeRaftServiceImpl final : public raft::v1::RaftService::Service
{
   public:
    grpc::Status RequestVote(grpc::ServerContext* /*ctx*/, const raft::v1::RequestVoteRequest* req,
                             raft::v1::RequestVoteResponse* resp) override
    {
        resp->set_term(req->term());
        resp->set_vote_granted(true);
        return grpc::Status::OK;
    }

    grpc::Status AppendEntries(grpc::ServerContext* /*ctx*/, const raft::v1::AppendEntriesRequest* req,
                               raft::v1::AppendEntriesResponse* resp) override
    {
        resp->set_term(req->term());
        resp->set_success(true);
        resp->set_match_index(req->prev_log_index());
        return grpc::Status::OK;
    }
};

static std::unique_ptr<grpc::Server> start_server(int port, std::unique_ptr<FakeRaftServiceImpl>& service_holder)
{
    service_holder = std::make_unique<FakeRaftServiceImpl>();

    grpc::ServerBuilder builder;
    std::string address = "127.0.0.1:" + std::to_string(port);
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(service_holder.get());
    return builder.BuildAndStart();
}

TEST(RaftGrpcTransportIntegration, ThreeNodes_RequestVoteAndAppendEntries)
{
    const std::vector<int> ports = {50051, 50052, 50053};
    std::vector<std::unique_ptr<FakeRaftServiceImpl>> services(ports.size());
    std::vector<std::unique_ptr<grpc::Server>> servers;
    servers.reserve(ports.size());

    for (size_t i = 0; i < ports.size(); ++i)
    {
        servers.push_back(start_server(ports[i], services[i]));
        ASSERT_NE(servers.back(), nullptr);
    }

    std::this_thread::sleep_for(300ms);

    auto sl = std::make_shared<utils::ServiceLocator>();
    sl->registerService<utils::ThreadPool>(4);

    std::vector<PeerInfo> peers;
    peers.reserve(ports.size());
    for (size_t i = 0; i < ports.size(); ++i)
    {
        peers.push_back(PeerInfo{
            static_cast<std::uint64_t>(i + 1),
            std::string("127.0.0.1:") + std::to_string(ports[i]),
        });
    }

    RaftGrpcTransport transport(peers, sl);
    transport.set_rpc_timeout(2000);

    RequestVoteRequestMsg rv_req{};
    rv_req.term = 42;
    rv_req.candidateId = 99;
    rv_req.lastLogIndex = 0;
    rv_req.lastLogTerm = 0;

    AppendEntriesRequestMsg ae_req{};
    ae_req.term = 43;
    ae_req.leaderId = 1;
    ae_req.prevLogIndex = 0;
    ae_req.prevLogTerm = 0;
    ae_req.leaderCommit = 0;

    std::mutex mtx;
    std::condition_variable cv;

    const int expected = static_cast<int>(peers.size());
    std::atomic<int> rv_count{0};
    std::atomic<int> ae_count{0};

    std::vector<RequestVoteResponseMsg> rv_results(peers.size());
    std::vector<AppendEntriesResponseMsg> ae_results(peers.size());

    auto idx_of = [&](int peerId) -> int
    {
        for (size_t i = 0; i < peers.size(); ++i)
        {
            if (static_cast<int>(peers[i].id) == peerId) return static_cast<int>(i);
        }
        return -1;
    };

    transport.broadcastRequestVote(rv_req,
                                   [&](int peerId, const RequestVoteResponseMsg& resp)
                                   {
                                       int idx = idx_of(peerId);
                                       if (idx >= 0) rv_results[idx] = resp;
                                       rv_count.fetch_add(1, std::memory_order_relaxed);
                                       std::lock_guard<std::mutex> lk(mtx);
                                       cv.notify_one();
                                   });

    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait_for(lk, 2s, [&] { return rv_count.load() >= expected; });
    }

    EXPECT_EQ(rv_count.load(), expected);
    for (int i = 0; i < expected; ++i)
    {
        EXPECT_EQ(rv_results[i].term, static_cast<std::uint64_t>(42));
        EXPECT_TRUE(rv_results[i].voteGranted);
    }

    transport.broadcastAppendEntries(ae_req,
                                     [&](int peerId, const AppendEntriesResponseMsg& resp)
                                     {
                                         int idx = idx_of(peerId);
                                         if (idx >= 0) ae_results[idx] = resp;
                                         ae_count.fetch_add(1, std::memory_order_relaxed);
                                         std::lock_guard<std::mutex> lk(mtx);
                                         cv.notify_one();
                                     });

    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait_for(lk, 2s, [&] { return ae_count.load() >= expected; });
    }

    EXPECT_EQ(ae_count.load(), expected);
    for (int i = 0; i < expected; ++i)
    {
        EXPECT_EQ(ae_results[i].term, static_cast<std::uint64_t>(43));
        EXPECT_TRUE(ae_results[i].success);
    }

    for (auto& srv : servers)
    {
        if (srv)
        {
            srv->Shutdown();
            srv->Wait();
        }
    }

    try
    {
        auto tp = sl->get<utils::ThreadPool>();
        tp->stop_and_wait();
    }
    catch (...)
    {
    }
}