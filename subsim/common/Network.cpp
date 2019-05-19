#include "Network.h"

#include "Exceptions.h" // For NetworkError
#include "Globals.h" // For various port/max client defines

#include "RakNetTypes.h" // For SocketDescriptor
#include "RakPeerInterface.h" // For RakPeer
#include "BitStream.h"

#include "Log.h"
#include "Messages.h" // For various message types
#include "Lobby.h"   // For LobbyStatus
#include "version.h"

#include <iostream>
#include <thread> // For std::this_thread
#include <chrono> // For std::chrono::milliseconds


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

void Network::registerCallback(ReceiveInterface* callback)
{
    if (callbacks.insert(callback).second == false)
    {
        Log::writeToLog(Log::WARN, "Callback class ", callback, "already registered! Ignoring.");
    } else {
        /* Set network pointer so the callback can call network functions */
        callback->network = this;
        Log::writeToLog(Log::L_DEBUG, "Registered callback class ", callback);
    }
}

void Network::deregisterCallback(ReceiveInterface* callback)
{
    auto it = callbacks.find(callback);

    if (it == callbacks.end())
    {
        Log::writeToLog(Log::ERR, "Attempted to remove callback ", callback, " that was not registered!");
        throw std::runtime_error("Removal of unregistered callback attempted!");
    }

    (*it)->network = nullptr;
    callbacks.erase(it);

    Log::writeToLog(Log::L_DEBUG, "Deregistered callback class ", callback);
}

void Network::sendMessage(RakNet::RakNetGUID destination, MessageInterface* message, PacketReliability reliability)
{
    // Check that this actually is a valid destination
    if (confirmedConnections.count(destination) == 0)
    {
        Log::writeToLog(Log::WARN, "Attempted to send a message of type:", message->getType(),
            " to invalid destination GUID:", destination);
        throw InvalidDestinationError("Attempted to send a message to invalid destination.");
    }

    RakNet::BitStream outStream;
    outStream.Write((RakNet::MessageID)message->getType());
    message->serialize(outStream);

    if (node->Send(&outStream, PacketPriority::MEDIUM_PRIORITY, reliability, message->getType(), destination, false) == 0)
    {
        Log::writeToLog(Log::ERR, "Unable to send message with type:", message->getType(), " to system ", destination);
        throw NetworkMessageError("Unable to send message to destination");
    }
}

/*!
 * Templated function that attempts to call a given function pointer
 * on each entry in a vector, assuming the function pointer returns a bool.
 *
 * The first entry in the vector that returns true "captures" the event, preventing future
 * modules from being informed about the event.
 *
 * Returns true if the event was handled, false if it was not. There might be different
 * relevant error states. Not handeling a information event may only be a warning,
 * whereas not handling a syncronization event could be fatal.
 */
template <typename T, typename ...Types>
bool tryCallbacks(std::set<ReceiveInterface*>& interfaces, T func, Types... args)
{
    for (auto it : interfaces)
    {
        if ((it->*func)(args...))
        {
            // We handled it! Stop trying handlers.
            return true;
        }
    }
    return false;
}

RakNet::RakNetGUID Network::getOurGUID()
{
    if (node)
    {  
        return node->GetMyGUID();
    }
    return RakNet::UNASSIGNED_RAKNET_GUID;
}

RakNet::RakNetGUID Network::getFirstConnectionGUID()
{
    if (confirmedConnections.size() > 0)
    {
        return *confirmedConnections.begin();
    }
    return RakNet::UNASSIGNED_RAKNET_GUID;
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

                /* Add this system to the verified connection list */
                confirmedConnections.insert(packet->guid);


                /* We successfully connected! Inform any waiting callbacks */
                if (!tryCallbacks(callbacks, &ReceiveInterface::ConnectionEstablished, packet->guid))
                {
                    Log::writeToLog(Log::WARN, "ConnectionEstablished callback not handled!");
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

                if (!tryCallbacks(callbacks, &ReceiveInterface::ConnectionLost, packet->guid))
                {
                    Log::writeToLog(Log::WARN, "ConnectionLost callback not handled!");
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

                if (!tryCallbacks(callbacks, &ReceiveInterface::ConnectionLost, packet->guid))
                {
                    Log::writeToLog(Log::WARN, "ConnectionLost callback not handled!");
                }
                break;
            }

            case ID_LOBBY_STATUS_REQUEST:
            {   
                LobbyStatusRequest newRequest(packetBs);

                if (!tryCallbacks(callbacks, &ReceiveInterface::LobbyStatusRequested, packet->guid, newRequest))
                {
                    Log::writeToLog(Log::ERR, "Incoming LobbyStatusRequest not handled!");
                    throw NetworkMessageUnhandledError("LobbyStatusRequested not handled!");
                }
                break;
            }

            case ID_LOBBY_STATUS:
            {
                LobbyStatus newStatus(packetBs);
                if (!tryCallbacks(callbacks, &ReceiveInterface::UpdatedLobbyStatus, newStatus))
                {
                    Log::writeToLog(Log::ERR, "Got unexpected/unhandled LobbyStatus!");
                    throw NetworkMessageUnhandledError("UpdatedLobbyStatus not handled!");
                }
                break;
            }

            case ID_ENVELOPE:
            {
                // creation of the envelope will automatically add the event to the queue if possible
                EnvelopeMessage eventMessage(packetBs, packet->guid);
                break;
            }

            default:
                Log::writeToLog(Log::WARN, "Unknown packet with id:" , packet->data[0], " recieved");
                break;
            }
        }
    }
}
