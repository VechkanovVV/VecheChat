#include "messaging_service.h"

#include "logger.h"

grpc::Status MessagingServiceImpl::SendToLeaderMessage(grpc::ServerContext*,
                                                       const messaging::v1::SendToLeaderMessageRequest* request,
                                                       messaging::v1::SendToLeaderMessageResponse* response)
{
    if (!core_ || !request || !response)
    {
        auto logger = Logger::getLogger();
        if (logger) logger->error("Invalid parameters in SendToLeaderMessage");
        return grpc::Status(grpc::StatusCode::INTERNAL, "bad core/request/response");
    }

    auto logger = Logger::getLogger();
    if (logger) logger->info("Received message for leader: {}", request->text());

    if (core_->self_id() == core_->leader_id())
    {
        core_->broadcast(request->text(), request->id(), [response](bool ok) { response->set_delivered(ok); });
    }
    else
    {
        std::cout << "Message: " << request->text() << std::endl;
        response->set_delivered(true);
    }
    return grpc::Status::OK;
}

grpc::Status MessagingServiceImpl::SendBroadcastMessage(grpc::ServerContext*,
                                                        const messaging::v1::SendBroadcastMessageRequest* request,
                                                        messaging::v1::SendBroadcastMessageResponse* response)
{
    if (!core_ || !request || !response)
    {
        auto logger = Logger::getLogger();
        if (logger) logger->error("Invalid parameters in SendBroadcastMessage");
        return grpc::Status(grpc::StatusCode::INTERNAL, "bad core/request/response");
    }

    auto logger = Logger::getLogger();
    if (logger) logger->info("Broadcast message received: {}", request->text());
    std::cout << "Broadcast: " << request->text() << std::endl;
    response->set_delivered(true);
    return grpc::Status::OK;
}