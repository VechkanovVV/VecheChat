#include "raft_server.h"

#include <fstream>
#include <sstream>
#include <thread>

#include "logger.h"

RaftServer::RaftServer(int node_id, const std::string& address, const std::vector<PeerInfo>& peers,
                       unsigned election_min, unsigned election_max, unsigned heartbeat,
                       std::shared_ptr<utils::ServiceLocator> sl)
    : node_id_(node_id),
      address_(address),
      peers_(peers),
      election_min_(election_min),
      election_max_(election_max),
      heartbeat_(heartbeat),
      sl_(sl)
{
}

bool RaftServer::start()
{
    try
    {
        Config cfg;
        cfg.nodeId = node_id_;
        cfg.peers = peers_;
        cfg.electionMinMs = election_min_;
        cfg.electionMaxMs = election_max_;
        cfg.heartbeatMs = heartbeat_;

        if (!peers_.empty())
        {
            transport_ = std::make_shared<RaftGrpcTransport>(peers_, sl_);
        }

        core_ = std::make_shared<RaftCore>(cfg, transport_);
        core_->start();

        service_ = std::make_unique<RaftServiceImpl>(core_, sl_, address_);

        grpc::ServerBuilder builder;
        builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
        builder.RegisterService(service_.get());

        server_ = builder.BuildAndStart();

        auto logger = Logger::getLogger();
        if (logger)
        {
            logger->info("Raft server started on {} with node ID: {}", address_, node_id_);
            if (!peers_.empty())
            {
                std::string peers_str;
                for (const auto& peer : peers_)
                {
                    peers_str += std::to_string(peer.id) + ":" + peer.address + " ";
                }
                logger->info("Peers: {}", peers_str);
            }
            else
            {
                logger->info("No peers configured (single-node mode)");
            }
            logger->info("Election timeout: {}-{}ms", election_min_, election_max_);
            logger->info("Heartbeat interval: {}ms", heartbeat_);
        }

        return true;
    }
    catch (const std::exception& e)
    {
        auto logger = Logger::getLogger();
        if (logger)
        {
            logger->error("Error starting server: {}", e.what());
        }
        return false;
    }
}

void RaftServer::stop()
{
    RemovePeerRequestMsg req;
    req.id = node_id_;

    auto role = core_->role();
    if (role == Role::Leader)
    {
        if (transport_) transport_->broadcastRemovePeer(req);
    }
    else
    {
        auto leader_id = core_->leader_id();
        if (leader_id && transport_)
        {
            transport_->sendRemovePeerToPeer(*leader_id, req);
        }
    }

    if (server_)
    {
        server_->Shutdown();
    }
    auto logger = Logger::getLogger();
    if (logger)
    {
        logger->info("Server stopped");
    }
}

std::string RaftServer::getLogs() const
{
    std::ifstream log_file("raft_node_" + std::to_string(node_id_) + ".log");
    if (!log_file.is_open())
    {
        return "No log file found";
    }

    std::stringstream buffer;
    buffer << log_file.rdbuf();
    return buffer.str();
}