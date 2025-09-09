#include "messaging_grpc_transport.h"

#include "logger.h"

using namespace std::chrono;

MessagingGrpcTransport::MessagingGrpcTransport(int self_id, const std::vector<PeerInfo>& peer_list,
                                               std::shared_ptr<utils::ServiceLocator> sl, int rpc_timeout_ms)
    : self_id_(self_id), rpc_timeout_ms_(rpc_timeout_ms > 0 ? rpc_timeout_ms : 2000), sl_(std::move(sl))
{
    auto logger = Logger::getLogger();
    std::lock_guard<std::mutex> lk(mtx_);
    for (const auto& p : peer_list)
    {
        auto ps = std::make_shared<PeerState>();
        ps->id = static_cast<int>(p.id);
        ps->address = p.address;
        ps->channel = grpc::CreateChannel(ps->address, grpc::InsecureChannelCredentials());

        auto deadline = system_clock::now() + milliseconds(500);
        if (ps->channel->WaitForConnected(deadline))
        {
            if (logger) logger->info("Connected to peer {} at {}", ps->id, ps->address);
        }
        else
        {
            if (logger) logger->warn("Failed to connect to peer {} at {}", ps->id, ps->address);
        }

        ps->stub = messaging::v1::MessagingService::NewStub(ps->channel);
        peers_.emplace(ps->id, std::move(ps));
    }
}

void MessagingGrpcTransport::sendBroadcastMessage(const std::string& text, std::int64_t id,
                                                  std::function<void(int peerId, bool ok)> onReply)
{
    auto logger = Logger::getLogger();
    std::vector<std::shared_ptr<PeerState>> targets;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        targets.reserve(peers_.size());
        for (auto& kv : peers_)
        {
            if (kv.first != self_id_)
            {
                targets.push_back(kv.second);
            }
        }
    }

    messaging::v1::SendBroadcastMessageRequest req;
    req.set_text(text);
    req.set_id(id);

    const int to = rpc_timeout_ms_;

    for (auto& ps : targets)
    {
        auto cb = onReply;
        auto ps_copy = ps;
        sl_->get<utils::ThreadPool>()->add_task(
            [ps_copy, req, cb, to, logger]
            {
                grpc::ClientContext ctx;
                ctx.set_deadline(system_clock::now() + milliseconds(to));
                messaging::v1::SendBroadcastMessageResponse resp;
                auto status = ps_copy->stub->SendBroadcastMessage(&ctx, req, &resp);
                bool ok = status.ok() && resp.delivered();
                if (!ok && logger)
                {
                    logger->warn("Failed to send broadcast to peer {}: {}", ps_copy->id, status.error_message());
                }
                if (cb) cb(ps_copy->id, ok);
            });
    }
}

void MessagingGrpcTransport::sendToLeaderMessage(int leaderPeerId, const std::string& text, std::int64_t id,
                                                 std::function<void(bool ok)> onReply)
{
    auto logger = Logger::getLogger();
    std::shared_ptr<PeerState> ps;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = peers_.find(leaderPeerId);
        if (it != peers_.end()) ps = it->second;
    }

    if (!ps || !ps->stub)
    {
        if (logger) logger->warn("Leader {} not found in peer list", leaderPeerId);
        if (onReply) onReply(false);
        return;
    }

    messaging::v1::SendToLeaderMessageRequest req;
    req.set_text(text);
    req.set_id(id);

    const int to = rpc_timeout_ms_;
    sl_->get<utils::ThreadPool>()->add_task(
        [ps, req, onReply, to, logger]
        {
            grpc::ClientContext ctx;
            ctx.set_deadline(system_clock::now() + milliseconds(to));
            messaging::v1::SendToLeaderMessageResponse resp;
            auto status = ps->stub->SendToLeaderMessage(&ctx, req, &resp);
            bool ok = status.ok() && resp.delivered();
            if (!ok && logger)
            {
                logger->warn("Failed to send to leader {}: {}", ps->id, status.error_message());
            }
            if (onReply) onReply(ok);
        });
}

void MessagingGrpcTransport::set_rpc_timeout(int ms)
{
    rpc_timeout_ms_ = ms;
}