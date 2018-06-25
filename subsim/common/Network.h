#pragma once

#include <string> // For std::string

/// Forward definition of RakPeerInterface
namespace RakNet
{
    class RakPeerInterface;
}

/*!
 * Network encapsulates network interactions with other clients. This abstracts
 * away the RakNet internals from the rest of the program. This allows us to make
 * some choices, such as network transport layer reliability, and have them not
 * affect the other clients.
 */
class Network
{
public:
    /// Starts up the internal RakNet interface, either in server or client mode
    Network(bool is_server = false);

    /// Shuts down the network connection on exit
    ~Network();

    /// Connects to a game master given a hostname or IP as a string
    void connect(const std::string& hostname);

private:
    RakNet::RakPeerInterface* node;
};
