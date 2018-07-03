#include "Network.h"

#include "Exceptions.h" // For NetworkError
#include "Globals.h" // For various port/max client defines

#include "RakNetTypes.h" // For SocketDescriptor
#include "RakPeerInterface.h" // For RakPeer
#include "BitStream.h"

#include "Log.h"
#include "Messages.h" // For various message types
#include "version.h"

#include <iostream>
#include <thread> // For std::this_thread
#include <chrono> // For std::chrono::milliseconds

/*!
 * Definition of an ostream override so that we can easily log
 * RakNetGUID's
 */
std::ostream& operator<< (std::ostream& stream, const RakNet::RakNetGUID& guid)
{
    return stream << RakNet::RakNetGUID::ToUint32(guid);
}

Network::Network(bool is_server)
    : shouldShutdown(false)
{
    node = RakNet::RakPeerInterface::GetInstance();

    // Use an empty socket descriptor if we're a client
    RakNet::SocketDescriptor sd = is_server ? 
        RakNet::SocketDescriptor(NETWORK_SERVER_PORT, 0) : RakNet::SocketDescriptor();
    unsigned short num_clients = is_server ? NETWORK_MAX_CLIENTS : 1;

    Log::writeToLog(Log::L_DEBUG, "Starting networking with ", num_clients, " possible active connections");

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

    /* Make sure people can connect to us */
    if (is_server)
    {
        node->SetMaximumIncomingConnections(NETWORK_MAX_CLIENTS);
    }

    Log::writeToLog(Log::L_DEBUG, "Starting networking thread");
    recieveThread = std::thread(&Network::handlePackets, this);
}

Network::~Network()
{
    Log::writeToLog(Log::INFO, "Shutting down networking");
    Log::writeToLog(Log::L_DEBUG, "Signaling networking thread to close...");
    /* Shutdown inside a locked mutex. Lock guard for exception safety */
    {
        std::lock_guard<std::mutex> lock(shutdownMutex);
        shouldShutdown = true;
    }
    // Wait for the networking thread to fully shutdown
    recieveThread.join();
    Log::writeToLog(Log::L_DEBUG, "Networking thread closed! Waiting for connections to close.");
    node->Shutdown(500);
    RakNet::RakPeerInterface::DestroyInstance(node);
    Log::writeToLog(Log::INFO, "Networking fully shutdown.");
}

void Network::connect(const std::string& hostname)
{
    Log::writeToLog(Log::L_DEBUG, "Attempting to connect to server:", hostname, " on port ", NETWORK_SERVER_PORT);
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
        /* Check for shutdown inside a locked mutex */
        {
            std::lock_guard<std::mutex> lock(shutdownMutex);
            if (shouldShutdown)
            {
                return;
            }
        }
        /* Sleep so we don't busy loop */
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        for (RakNet::Packet* packet = node->Receive(); packet; node->DeallocatePacket(packet), packet = node->Receive())
        {
            RakNet::BitStream packetBs(packet->data + 1, packet->length - 1, false);
            switch (packet->data[0])
            {
            case ID_CONNECTION_REQUEST_ACCEPTED:
            {
                Log::writeToLog(Log::L_DEBUG, "Successfully connected to system GUID:", packet->guid);
                // Send a version message to the other computer to validate.
                VersionMessage ourVersion(VERSION_MAJOR, VERSION_MINOR);
                RakNet::BitStream out;

                out.Write((RakNet::MessageID)ID_VERSION);
                ourVersion.serialize(out);
                node->Send(&out, HIGH_PRIORITY, RELIABLE, 0, packet->guid, false);
                break;
            }

            case ID_NEW_INCOMING_CONNECTION:
            {
                Log::writeToLog(Log::L_DEBUG, "System GUID:", packet->guid, " connected to us!");
                // Send a version message to the other computer to validate.
                VersionMessage ourVersion(VERSION_MAJOR, VERSION_MINOR);
                RakNet::BitStream out;

                out.Write((RakNet::MessageID)ID_VERSION);
                ourVersion.serialize(out);
                node->Send(&out, HIGH_PRIORITY, RELIABLE, 0, packet->guid, false);
                break;
            }

            case ID_VERSION:
            {
                VersionMessage otherVersion(packetBs);
                Log::writeToLog(Log::L_DEBUG, "System GUID:", packet->guid, " connected with verison ",
                    otherVersion.versionMajor, ".", otherVersion.versionMinor);

                if (otherVersion.versionMajor != VERSION_MAJOR || otherVersion.versionMinor != VERSION_MINOR)
                {
                    Log::writeToLog(Log::WARN, "System GUID:", packet->guid, " has version mismatch! Disconnecting");
                    /* Inform the other computer of the verion mismatch then disconnect */
                    VersionMessage ourVersion(VERSION_MAJOR, VERSION_MINOR);
                    RakNet::BitStream out;

                    out.Write((RakNet::MessageID)ID_VERSION_MISMATCH);
                    ourVersion.serialize(out);
                    node->Send(&out, IMMEDIATE_PRIORITY, RELIABLE, 0, packet->guid, false);

                    // hangup nicely
                    node->CloseConnection(packet->guid, true);

                    break;
                }
                break;
            }

            case ID_ALREADY_CONNECTED:
                Log::writeToLog(Log::WARN, "Attempted to connect to a computer already connected to!");
                break;

            case ID_NO_FREE_INCOMING_CONNECTIONS:
                Log::writeToLog(Log::ERR, "Server full! Unable to add another connection.");
                throw NetworkConnectionError("Server full!");
                break;

            case ID_DISCONNECTION_NOTIFICATION:
            {
                Log::writeToLog(Log::L_DEBUG, "System GUID:", packet->guid, " disconnected gracefully.");
                // Remove this from our confirmed connections list
                auto foundIt = confirmedConnections.find(packet->guid);
                if (foundIt != confirmedConnections.end())
                {
                    confirmedConnections.erase(foundIt);
                }
                break;
            }

            case ID_CONNECTION_LOST:
            {
                Log::writeToLog(Log::L_DEBUG, "System GUID:", packet->guid, " disconnected rudely.");
                // Remove this from our confirmed connections list
                auto foundIt = confirmedConnections.find(packet->guid);
                if (foundIt != confirmedConnections.end())
                {
                    confirmedConnections.erase(foundIt);
                }
                break;
            }

            default:
                Log::writeToLog(Log::WARN, "Unknown packet with id:" , packet->data[0], " recieved");
                break;
            }
        }
    }
}
