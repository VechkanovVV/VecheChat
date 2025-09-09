#include <iostream>
#include <string>
#include <thread>

#include "logger.h"
#include "raft_server.h"
#include "service_locator.h"
#include "thread_pool.h"

bool parse_args(int argc, char** argv, int& node_id, std::string& address, std::vector<PeerInfo>& peers,
                unsigned& election_min, unsigned& election_max, unsigned& heartbeat)
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <node_id> <address:port> <peer1_id:address:port> [peer2_id:address:port] ... "
                  << "[--election_min <ms>] [--election_max <ms>] [--heartbeat <ms>]" << std::endl;
        return false;
    }

    node_id = std::stoi(argv[1]);
    address = argv[2];

    for (int i = 3; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--election_min" && i + 1 < argc)
        {
            election_min = std::stoul(argv[++i]);
            continue;
        }
        else if (arg == "--election_max" && i + 1 < argc)
        {
            election_max = std::stoul(argv[++i]);
            continue;
        }
        else if (arg == "--heartbeat" && i + 1 < argc)
        {
            heartbeat = std::stoul(argv[++i]);
            continue;
        }

        size_t first_colon = arg.find(':');
        size_t last_colon = arg.rfind(':');

        if (first_colon == std::string::npos || last_colon == std::string::npos || first_colon == last_colon)
        {
            std::cerr << "Invalid peer format: " << arg << ". Expected: <id>:<address>:<port>" << std::endl;
            return false;
        }

        try
        {
            uint64_t peer_id = std::stoull(arg.substr(0, first_colon));
            std::string peer_address = arg.substr(first_colon + 1);

            peers.push_back({peer_id, peer_address});
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error parsing peer: " << arg << ", error: " << e.what() << std::endl;
            return false;
        }
    }

    return true;
}

int main(int argc, char** argv)
{
    int node_id;
    std::string address;
    std::vector<PeerInfo> peers;
    unsigned election_min = 150;
    unsigned election_max = 300;
    unsigned heartbeat = 50;

    if (argc > 1)
    {
        if (!parse_args(argc, argv, node_id, address, peers, election_min, election_max, heartbeat))
        {
            return 1;
        }
    }
    else
    {
        node_id = 1;
        address = "0.0.0.0:50051";
        std::cout << "No arguments provided, starting in single-node mode on " << address << std::endl;
    }

    Logger::init(std::to_string(node_id));

    auto sl = std::make_shared<utils::ServiceLocator>();
    sl->registerService<utils::ThreadPool>(std::thread::hardware_concurrency());

    RaftServer server(node_id, address, peers, election_min, election_max, heartbeat, sl);
    if (!server.start())
    {
        return 1;
    }

    std::string command;
    while (true)
    {
        std::cout << "raft> ";
        std::getline(std::cin, command);

        if (command == "stop")
        {
            server.stop();
            break;
        }
        else if (command == "get_log")
        {
            std::cout << server.getLogs() << std::endl;
        }
        else if (command == "help")
        {
            std::cout << "Available commands:" << std::endl;
            std::cout << "  stop      - Stop the server" << std::endl;
            std::cout << "  get_log   - Get server logs" << std::endl;
            std::cout << "  help      - Show this help" << std::endl;
            std::cout << "  exit      - Exit the CLI" << std::endl;
        }
        else if (command == "exit")
        {
            break;
        }
        else if (!command.empty())
        {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }

    return 0;
}