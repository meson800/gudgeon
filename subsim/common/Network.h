#pragma once

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
    Network(bool is_server);

    /// Connects to another 
};
