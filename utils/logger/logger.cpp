#include "logger.h"

#include <iostream>

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;

void Logger::init(const std::string& node_id)
{
    try
    {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("raft_node_" + node_id + ".log", true);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [node " + node_id + "] %v");

        std::vector<spdlog::sink_ptr> sinks{file_sink};
        logger_ = std::make_shared<spdlog::logger>("raft_logger", sinks.begin(), sinks.end());

        logger_->set_level(spdlog::level::info);
        logger_->flush_on(spdlog::level::info);

        spdlog::register_logger(logger_);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
    }
}

std::shared_ptr<spdlog::logger> Logger::getLogger()
{
    return logger_;
}