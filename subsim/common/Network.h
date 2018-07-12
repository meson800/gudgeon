#pragma once

#include <string> // For std::string
#include <set>
#include <thread>
#include <mutex>

#include <iostream>

#include "RakNetTypes.h" // For RakNetGUID
#include "PacketPriority.h" // For PacketReliability

/// Forward definition of RakPeerInterface
namespace RakNet
{
    class RakPeerInterface;
}

/// Forward declaration of LobbyStatus
class LobbyStatus;
/// Forward declaration of LobbyStatusRequest
class LobbyStatusRequest;

/// Forward declaration of Network
class Network;

/// Forward declaration of MessageInterface
class MessageInterface;

/*!
 * Definition of an ostream override so that we can easily log
 * RakNetGUID's
 */
static std::ostream& operator<< (std::ostream& stream, const RakNet::RakNetGUID& guid)
{
    return stream << RakNet::RakNetGUID::ToUint32(guid);
}

/*!
 * RecieveInterface is a virtual class from which objects interested in recieving
 * simulation messages should derive. A pointer to a class of this type is passed
 * to the main Network thread, who then uses member functions to send callbacks to.
 *
 * Callbacks are not implemented as pure virtual functions, as the default action
 * for the callbacks should be to drop the packet (return false).
 *
 * If a callback successfully handled a packet, the callback function should return true.
 *
 * Network will iterate through the list of registered callback classes, and emit a warning
 * if a callback is unhandled. Callback propogation stops when the first callback class successfully
 * handles the message.
 */
class ReceiveInterface
{
public:
    ReceiveInterface() : network(nullptr) {}

    /**
     * Callback called when a Connect call has succeeded. Client can now send messages
     * to the listed node
     *
     * This callback is called after version verification has succeeded.
     */
    virtual bool ConnectionEstablished(RakNet::RakNetGUID other) { return false; }

    /**
     * Callback called when a client has disconnected.
     *
     * Function argument is the disconnected system GUID.
     */
    virtual bool ConnectionLost(RakNet::RakNetGUID other) { return false; }

    /**
     * Callback used when a connected client requests lobby status.
     *
     * Function argument is the requesting system.
     */
    virtual bool LobbyStatusRequested(RakNet::RakNetGUID other, const LobbyStatusRequest& status) { return false; }

    /**
     * Callback for when the lobby status has changed.
     */
    virtual bool UpdatedLobbyStatus(const LobbyStatus& status) { return false; }

    /**
     * Member variable that is set equal to the Network of which it is registered.
     */
    Network* network;

};


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

    /// Adds a callback class to the registered callback list
    void registerCallback(ReceiveInterface* callback);

    /// Removes a callback class from the registered callback list
    void deregisterCallback(ReceiveInterface* callback);

    /**
     * Sends a message to the specified client.
     * Throws an InvalidDestinationError if the GUID given isn't a 'confirmed' connection
     *
     * Takes a polymorphic type that inherits from MessageInterface
     */
    void sendMessage(RakNet::RakNetGUID destination, MessageInterface* message, PacketReliability reliability);



private:
    RakNet::RakPeerInterface* node;

    /// Member thread that handles recieving messages from the queue.
    std::thread recieveThread;

    /// Member variables storing all currently registered callbacks
    std::set<ReceiveInterface*> callbacks;

    /// Member variable indicating when the networking thread should shutdown, along with protective mutex
    std::mutex shutdownMutex;
    bool shouldShutdown;

    /** Member variable storing all "confirmed" connections.
     * A connection is confirmed when the version number has been validated to be the same
     * between computers.
     */
    std::set<RakNet::RakNetGUID> confirmedConnections;

    /**
     * Main thread function. This handles packets as they come in, and notifies
     * registered callbacks of any changes.
     */
    void handlePackets();
};
