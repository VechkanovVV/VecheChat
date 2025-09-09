#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "imessaging_transport.h"
#include "service_locator.h"

class MessagingCore
{
   public:
    MessagingCore(int self_id, std::shared_ptr<utils::ServiceLocator> sl,
                  std::shared_ptr<IMessagingTransport> transport);

    void send(int leader_id, const std::string& text, std::int64_t id, std::function<void(bool ok)> onDone);
    void broadcast(const std::string& text, std::int64_t id, std::function<void(bool ok)> onDone);
    int self_id() const noexcept;
    int leader_id() const noexcept;

   private:
    int self_id_{};
    std::shared_ptr<utils::ServiceLocator> sl_;
    std::shared_ptr<IMessagingTransport> transport_;
};