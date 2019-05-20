#include "SimulationMaster.h"
#include <sstream>

#include "../common/Log.h"

SimulationMaster::SimulationMaster(Network* network_)
    : shouldShutdown(false)
    , network(network_)
    , EventReceiver({
        dispatchEvent<SimulationMaster, SimulationStartServer, &SimulationMaster::simStart>,
        dispatchEvent<SimulationMaster, ThrottleEvent, &SimulationMaster::throttle>,
    })
{
    lobbyInit = std::unique_ptr<LobbyHandler>(new LobbyHandler());
    network->registerCallback(lobbyInit.get());

}

SimulationMaster::~SimulationMaster()
{
    Log::writeToLog(Log::INFO, "Simulation master shutting down the simulation thread...");
    shouldShutdown = true;
    if (simLoop.joinable())
    {
        simLoop.join();
    }
    Log::writeToLog(Log::INFO, "Simulation thread shutdown successfully.");
}

void SimulationMaster::runSimLoop()
{
    Log::writeToLog(Log::INFO, "Main simulation loop started!");

    while (!shouldShutdown)
    {
        //TODO: possibly replace with a sleep until to keep game logic on a schedule?
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::lock_guard<std::mutex> lock(stateMux);
        
        // Deliver latest UnitState to every attached client
        for (const auto& teamPair : unitStates) {
            for (const UnitState &unitState : teamPair.second) {
                // This will send a redundant message if the same client is
                // handling multiple stations for a single unit, but it doesn't
                // matter
                for (const auto &stationPair : assignments[unitState.team][unitState.unit]) {
                    EnvelopeMessage envelope(unitState, stationPair.second);
                    EventSystem::getGlobalInstance()->queueEvent(envelope);
                }
            }
        }
    }
}

HandleResult SimulationMaster::simStart(SimulationStartServer* event)
{
    assignments = event->assignments;

    std::ostringstream sstream;
    for (auto& teamPair : assignments)
    {
        sstream << "Team " << teamPair.first << ": {";
        for (auto& units : teamPair.second)
        {
            sstream << "{";
            for (auto& stationPair : units)
            {
                sstream << StationNames[stationPair.first] << "->" << stationPair.second << ", ";
            }
            sstream << "}";
        }
        sstream << "}";
    }
    
    for (auto& teamPair : assignments)
    {
        unitStates[teamPair.first] = std::vector<UnitState>();
        for (uint32_t unit = 0; unit < teamPair.second.size(); ++unit) {
            UnitState unitState;
            unitState.team = teamPair.first;
            unitState.unit = unit;
            unitState.tubeIsArmed = std::vector<bool>(5, false);
            unitState.tubeOccupancy = std::vector<UnitState::TubeStatus>(5, UnitState::Empty);
            unitState.remainingTorpedos = 10;
            unitState.remainingMines = 10;
            unitState.torpedoDistance = 100;
            unitState.x = 0;
            unitState.y = 0;
            unitState.depth = 0;
            unitState.heading = 0;
            unitState.pitch = 0;
            unitState.speed = 0;
            unitState.powerAvailable = 100;
            unitState.powerUsage = 0;
            unitState.yawEnabled = true;
            unitState.pitchEnabled = true;
            unitState.engineEnabled = true;
            unitState.commsEnabled = true;
            unitState.sonarEnabled = true;
            unitState.weaponsEnabled = true;
            unitStates[teamPair.first].push_back(unitState);
        }
    }

    Log::writeToLog(Log::INFO, "Starting server-side simulation. Final assignments:", sstream.str());
    // Unhook the lobby handler and destroy it
    network->deregisterCallback(lobbyInit.get());
    lobbyInit.reset();

    // Start the game loop
    Log::writeToLog(Log::L_DEBUG, "Simulation master attempting to start simulation thread...");
    simLoop = std::thread(&SimulationMaster::runSimLoop, this);

    return HandleResult::Stop;
}

/// Handles the event when a submarine changes its throttle
HandleResult SimulationMaster::throttle(ThrottleEvent *event)
{
    {
        std::lock_guard<std::mutex> lock(stateMux);
        unitStates[event->team][event->unit].speed = event->speed;
    }

    return HandleResult::Stop;
}

