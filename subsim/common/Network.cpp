#include "Network.h"

#include "Exceptions.h" // For NetworkError
#include "Globals.h" // For various port/max client defines

#include "RakNetTypes.h" // For SocketDescriptor
#include "RakPeerInterface.h" // For RakPeer

Network::Network(bool is_server)
{
    node = RakNet::RakPeerInterface::GetInstance();

    // Use an empty socket descriptor if we're a client
    RakNet::SocketDescriptor sd = is_server ? 
        RakNet::SocketDescriptor(NETWORK_SERVER_PORT, 0) : RakNet::SocketDescriptor();
    unsigned short num_clients = is_server ? NETWORK_MAX_CLIENTS : 1;

    if (node->Startup(num_clients, &sd, 1, 0) != RakNet::RAKNET_STARTED) // last zero is thread priority 0
    {
        throw NetworkStartupError("Couldn't start networking!");
    }
}

Network::~Network()
{
    RakNet::RakPeerInterface::DestroyInstance(node);
}

void Network::connect(const std::string& hostname)
{
    if (node->Connect(hostname.c_str(), NETWORK_SERVER_PORT, 0, 0) != RakNet::CONNECTION_ATTEMPT_STARTED)
    {
        throw NetworkConnectionError("Couldn't initiate connection to remote host!");
    }
}
