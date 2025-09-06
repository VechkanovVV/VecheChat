#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct LogEntry
{
    std::uint64_t term{};
    std::vector<std::uint8_t> data;
};

struct RequestVoteRequestMsg
{
    std::uint64_t term{};
    int candidateId{};
    std::uint64_t lastLogIndex{};
    std::uint64_t lastLogTerm{};
};

struct RequestVoteResponseMsg
{
    std::uint64_t term{};
    bool voteGranted{};
};

struct AppendEntriesRequestMsg
{
    std::uint64_t term{};
    int leaderId{};
    std::uint64_t prevLogIndex{};
    std::uint64_t prevLogTerm{};
    std::vector<LogEntry> entries;
    std::uint64_t leaderCommit{};
};

struct AppendEntriesResponseMsg
{
    std::uint64_t term{};
    bool success{};
    std::uint64_t matchIndex{};
};