#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>

#include "itimer_strategy.h"

class ElectionTimerStrategy final : public utils::ITimerStrategy
{
   public:
    ElectionTimerStrategy(std::function<void()> on_election, unsigned int min_ms, unsigned int max_ms,
                          std::optional<unsigned> seed = std::nullopt)
        : on_election_(std::move(on_election)), dist_(min_ms, max_ms), rng_(seed ? *seed : std::random_device{}())
    {
        if (max_ms < min_ms) throw std::invalid_argument("min_ms > max_ms");
    }

    std::chrono::milliseconds nextTimeout() override { return std::chrono::milliseconds(dist_(rng_)); }

    void onTimeout() override
    {
        if (on_election_) on_election_();
    }

   private:
    std::function<void()> on_election_;
    std::uniform_int_distribution<unsigned int> dist_;
    std::mt19937 rng_;
};

class HeartbeatTimerStrategy final : public utils::ITimerStrategy
{
   public:
    explicit HeartbeatTimerStrategy(std::function<void()> on_send_heartbeat, unsigned int interval_ms)
        : on_send_heartbeat_(std::move(on_send_heartbeat)), interval_(interval_ms)
    {
        if (interval_ms == 0) throw std::invalid_argument("interval_ms == 0");
    }

    std::chrono::milliseconds nextTimeout() override { return std::chrono::milliseconds(interval_); }

    void onTimeout() override
    {
        if (on_send_heartbeat_) on_send_heartbeat_();
    }

   private:
    std::function<void()> on_send_heartbeat_;
    unsigned int interval_;
};