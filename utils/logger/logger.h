#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

class Logger
{
   public:
    static void init(const std::string& node_id);
    static std::shared_ptr<spdlog::logger> getLogger();

   private:
    static std::shared_ptr<spdlog::logger> logger_;
};