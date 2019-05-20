#pragma once
#include "../common/Network.h"
#include "../common/SimulationEvents.h"
#include "LobbyHandler.h"

#include <memory>
#include <thread>

typedef uint32_t TorpedoID;
typedef uint32_t MineID;

/*!
 * Class that stores what information we need for torpedos
 */
struct TorpedoState
{
    /// Stores the location of the torpedo.
    int64_t x, y, depth;

    /// Stores the current heading of the torpedo in degrees.
    uint16_t heading;

    // All torpedos travel at the same speed; we don't have a speed entry here because of this
};

/*!
 * Stores the location of mines.
 */
struct MineState
{
    /// Just store the location of mines; they don't move
    int64_t x, y, depth;
};


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

    /// Stops the game loop upon destruction
    ~SimulationMaster();

    /// Handles the event spawned when the lobby is full, and the game is starting
    HandleResult simStart(SimulationStartServer* event);
    
    /// Handles the event when a submarine changes its throttle
    HandleResult throttle(ThrottleEvent *event);

    /// Handles the event when the submarine steers left or right
    HandleResult steering(SteeringEvent *event);

private:
    /// Internal game simulation function. Runs continuously in its own thread
    void runSimLoop();
    
    /// Helper function for runSimLoop
    void runSimForUnit(UnitState *unitState);

    /// Thread for the sim loop
    std::thread simLoop;

    /// Shutdown flag atomic
    std::atomic<bool> shouldShutdown;

    /// Mutex to protect internal stsate
    std::mutex stateMux;

    /// Stores the internal pointer to the network subsystem
    Network* network;

    /// Smart pointer for the lobby handler. This is so we can deconstruct it when we're done with it.
    std::unique_ptr<LobbyHandler> lobbyInit;

    /// Internal mapping of teams/units/stations
    std::map<uint32_t, std::vector<std::vector<std::pair<StationType, RakNet::RakNetGUID>>>> assignments;
    std::set<RakNet::RakNetGUID> all_clients;

    /// Internal unit states
    std::map<uint32_t, std::vector<UnitState>> unitStates;

    /// Stores the current state of all torpedos
    std::map<uint32_t, TorpedoState> torpedos;

    /// Stores the current location of all mines
    std::map<uint32_t, MineState> mines;
};

