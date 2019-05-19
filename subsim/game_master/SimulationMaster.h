#pragma once
#include "../common/Network.h"
#include "../common/SimulationEvents.h"
#include "LobbyHandler.h"

#include <memory>

/*!
 * This class controls the entire simulation from the server-side. It first spawns
 * an instance of a LobbyHandler to get clients, but once it acquries enough clients
 * to start the game, it switches over to simulation mode.
 */
class SimulationMaster : public EventReceiver
{
public:
    /// Takes an initalized Network instance, in order to communicate with clients.
    SimulationMaster(Network* network);

    /// Handles the event spawned when the lobby is full, and the game is starting
    HandleResult simStart(SimulationStartServer* event);

private:
    /// Stores the internal pointer to the network subsystem
    Network* network;

    /// Smart pointer for the lobby handler. This is so we can deconstruct it when we're done with it.
    std::unique_ptr<LobbyHandler> lobbyInit;

    /// Internal mapping of teams/units/stations
    std::map<uint32_t, std::vector<std::vector<std::pair<StationType, RakNet::RakNetGUID>>>> assignments;
};

