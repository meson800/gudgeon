#include "Network.h"

#include "Exceptions.h" // For NetworkError
#include "Globals.h" // For various port/max client defines

#include "RakNetTypes.h" // For SocketDescriptor
#include "RakPeerInterface.h" // For RakPeer

#include "Log.h"

Network::Network(bool is_server)
{
    node = RakNet::RakPeerInterface::GetInstance();

    // Use an empty socket descriptor if we're a client
    RakNet::SocketDescriptor sd = is_server ? 
        RakNet::SocketDescriptor(NETWORK_SERVER_PORT, 0) : RakNet::SocketDescriptor();
    unsigned short num_clients = is_server ? NETWORK_MAX_CLIENTS : 1;

    if (node->Startup(num_clients, &sd, 1, 0) != RakNet::RAKNET_STARTED) // last zero is thread priority 0
    {
        if (is_server)
        {
            Log::writeToLog(Log::FATAL, "Couldn't start networking as the server! Tried to bind to port ", NETWORK_SERVER_PORT);
        } else {
            Log::writeToLog(Log::FATAL, "Couldn't start networking as a client!");
        }

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
        Log::writeToLog(Log::ERR, "Couldn't connect to server: ", hostname, " on port ", NETWORK_SERVER_PORT);
        throw NetworkConnectionError("Couldn't initiate connection to remote host!");
    }
}

void Network::registerCallback(RecieveInterface* callback)
{
    if (callbacks.insert(callback).second == false)
    {
        Log::writeToLog(Log::WARN, "Callback class ", callback, "already registered! Ignoring.");
    } else {
        Log::writeToLog(Log::L_DEBUG, "Registered callback class ", callback);
    }
}

void Network::deregisterCallback(RecieveInterface* callback)
{
    auto it = callbacks.find(callback);

    if (it == callbacks.end())
    {
        Log::writeToLog(Log::ERR, "Attempted to remove callback ", callback, " that was not registered!");
        throw std::runtime_error("Removal of unregistered callback attempted!");
    }

    Log::writeToLog(Log::L_DEBUG, "Deregistered callback class ", callback);
}

void Network::handlePackets()
{
    while (true)
    {
        for (RakNet::Packet* packet = node->Receive(); packet; node->DeallocatePacket(packet), packet = node->Receive())
        {
            switch (packet->data[0])
            {
            default:
                Log::writeToLog(Log::WARN, "Unknown packet with id:" , packet->data[0], " recieved");
                break;
            }
        }
    }
}
