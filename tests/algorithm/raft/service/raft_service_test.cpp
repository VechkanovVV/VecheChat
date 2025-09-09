#include "raft_service.h"

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "raft_core.h"
#include "service_locator.h"

using namespace std::chrono_literals;

static std::unique_ptr<grpc::Server> start_server(int port, std::unique_ptr<RaftServiceImpl>& service_holder,
                                                  const std::shared_ptr<RaftCore>& core,
                                                  const std::shared_ptr<utils::ServiceLocator>& sl,
                                                  const std::string& node_address)
{
    service_holder = std::make_unique<RaftServiceImpl>(core, sl, node_address);

    grpc::ServerBuilder builder;
    std::string address = "127.0.0.1:" + std::to_string(port);
    int selected_port = 0;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials(), &selected_port);
    builder.RegisterService(service_holder.get());
    auto server = builder.BuildAndStart();
    if (selected_port == 0) return nullptr;
    return server;
}

TEST(RaftServiceIntegration, ThreeNodes_RequestVote_AppendEntries)
{
    const std::vector<int> ports = {50061, 50062, 50063};

    auto sl = std::make_shared<utils::ServiceLocator>();

    std::vector<PeerInfo> peers_all;
    peers_all.reserve(ports.size());
    for (size_t i = 0; i < ports.size(); ++i)
    {
        peers_all.push_back(
            PeerInfo{static_cast<std::uint64_t>(i + 1), std::string("127.0.0.1:") + std::to_string(ports[i])});
    }

    std::vector<std::unique_ptr<RaftServiceImpl>> services(ports.size());
    std::vector<std::unique_ptr<grpc::Server>> servers;
    std::vector<std::shared_ptr<RaftCore>> cores;
    servers.reserve(ports.size());
    cores.reserve(ports.size());

    for (size_t i = 0; i < ports.size(); ++i)
    {
        Config cfg;
        cfg.nodeId = static_cast<int>(i + 1);
        cfg.peers = peers_all;

        std::shared_ptr<IRaftTransport> transport;  // nullptr for MVP: we call RPC handlers directly
        auto core = std::make_shared<RaftCore>(cfg, transport);
        cores.push_back(core);

        auto srv = start_server(ports[i], services[i], core, sl, peers_all[i].address);
        ASSERT_NE(srv, nullptr) << "Failed to bind port " << ports[i];
        servers.push_back(std::move(srv));
    }

    for (size_t i = 0; i < ports.size(); ++i)
    {
        const std::string target = "127.0.0.1:" + std::to_string(ports[i]);
        auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        ASSERT_TRUE(channel->WaitForConnected(std::chrono::system_clock::now() + 2s))
            << "Channel not ready: " << target;
        auto stub = raft::v1::RaftService::NewStub(channel);

        {
            grpc::ClientContext ctx;
            ctx.set_deadline(std::chrono::system_clock::now() + 2s);
            raft::v1::RequestVoteRequest req;
            req.set_term(1);
            req.set_candidate_id(100 + static_cast<int>(i));
            req.set_last_log_index(0);
            req.set_last_log_term(0);
            raft::v1::RequestVoteResponse resp;
            auto status = stub->RequestVote(&ctx, req, &resp);
            ASSERT_TRUE(status.ok()) << "RequestVote failed: " << status.error_message();
        }

        {
            grpc::ClientContext ctx;
            ctx.set_deadline(std::chrono::system_clock::now() + 2s);
            raft::v1::AppendEntriesRequest req;
            req.set_term(2);
            req.set_leader_id(1);
            req.set_prev_log_index(0);
            req.set_prev_log_term(0);
            req.set_leader_commit(0);
            raft::v1::AppendEntriesResponse resp;
            auto status = stub->AppendEntries(&ctx, req, &resp);
            ASSERT_TRUE(status.ok()) << "AppendEntries failed: " << status.error_message();
        }

        {
            grpc::ClientContext ctx;
            ctx.set_deadline(std::chrono::system_clock::now() + 2s);
            raft::v1::GetLeaderRequest req;
            raft::v1::GetLeaderResponse resp;
            auto status = stub->GetLeader(&ctx, req, &resp);
            ASSERT_TRUE(status.ok()) << "GetLeader failed: " << status.error_message();
        }
    }

    for (auto& s : servers)
    {
        if (s)
        {
            s->Shutdown();
            s->Wait();
        }
    }
}