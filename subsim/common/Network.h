#pragma once

#include <string> // For std::string
#include <set>
#include <thread>
#include <mutex>

#include <iostream>

#include "RakNetTypes.h" // For RakNetGUID

/// Forward definition of RakPeerInterface
namespace RakNet
{
    class RakPeerInterface;
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
class RecieveInterface
{
public:
    /**
     * Callback called when a Connect call has succeeded. Client can now send messages
     * to the listed node
     */
    bool ConnectionEstablished(RakNet::RakNetGUID other) { return false; }
    /**
     * Callback called when another system has attempted to connect to us. Messages
     * can now be sent to the other system.
     */
    bool IncomingConnection(RakNet::RakNetGUID other) { return false; }
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
    void registerCallback(RecieveInterface* callback);

    /// Removes a callback class from the registered callback list
    void deregisterCallback(RecieveInterface* callback);

private:
    RakNet::RakPeerInterface* node;

    /// Member thread that handles recieving messages from the queue.
    std::thread recieveThread;

    /// Member variables storing all currently registered callbacks
    std::set<RecieveInterface*> callbacks;

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
