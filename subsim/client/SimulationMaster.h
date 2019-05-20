#pragma once
#include "../common/Network.h"
#include "../common/SimulationEvents.h"
#include "../common/Network.h"

#include "LobbyHandler.h"
#include "TacticalStation.h"

#include <memory>

/*!
 * This class controls the entire simulation from the client-side. It first spawns
 * an instance of a LobbyHandler to get assignments, but once it gets its assignments
 * back from the server, it spawns a class for each station we are responsible for.
 */
class SimulationMaster : public EventReceiver, public ReceiveInterface
{
public:
    /// Takes an initalized Network instance, in order to communicate with clients.
    SimulationMaster(Network* network);

    /// Handles the event spawned when the lobby is full, and the game is starting
    HandleResult simStart(SimulationStart* event);

    /// Connection callback that is spawned when we have succesfully connected to a server. Spawns a Lobby instance
    virtual bool ConnectionEstablished(RakNet::RakNetGUID other) override;

    /// Connection callback that is called when we lose connection
    virtual bool ConnectionLost(RakNet::RakNetGUID other) override;

private:
    /// Destroys the lobbyInit pointer.
    void destroyLobby();

    /// Creates new stations once we have a station list
    void createStations();

    /// Stores the internal pointer to the network subsystem
    Network* network;

    /// Smart pointer for the lobby handler. This is so we can deconstruct it when we're done with it.
    std::unique_ptr<LobbyHandler> lobbyInit;

    /// Vector storing TacticalStations
    std::vector<std::shared_ptr<TacticalStation>> tactical;

    /// Internal mapping of teams/units/stations
    std::vector<SimulationStart::Station> stations;
};

