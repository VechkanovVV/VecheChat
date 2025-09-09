#pragma once
#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>

#include "messaging.grpc.pb.h"
#include "messaging_core.h"

class MessagingServiceImpl final : public messaging::v1::MessagingService::Service
{
   public:
    explicit MessagingServiceImpl(std::shared_ptr<MessagingCore> core) : core_(std::move(core)) {}

    grpc::Status SendToLeaderMessage(grpc::ServerContext*, const messaging::v1::SendToLeaderMessageRequest* request,
                                     messaging::v1::SendToLeaderMessageResponse* response) override;

    grpc::Status SendBroadcastMessage(grpc::ServerContext*, const messaging::v1::SendBroadcastMessageRequest* request,
                                      messaging::v1::SendBroadcastMessageResponse* response) override;

   private:
    std::shared_ptr<MessagingCore> core_;
};