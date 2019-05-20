#include "SimulationMaster.h"
#include <sstream>
#include <math.h>

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

        SonarDisplayState sonar;

        for (auto& torpedoPair : torpedos)
        {
            TorpedoState &torpedo = torpedoPair.second;
            constexpr int torpedo_speed = 10;
            torpedo.x += torpedo_speed * cos(torpedo.heading * 2*M_PI/360.0);
            torpedo.y += torpedo_speed * sin(torpedo.heading * 2*M_PI/360.0);
            sonar.addDot(torpedo.x, torpedo.y);
        }

        for (auto& teamPair : unitStates)
        {
            for (UnitState &unitState : teamPair.second)
            {
                runSimForUnit(&unitState);

                sonar.addDot(unitState.x, unitState.y);

                // Deliver latest UnitState to every attached client
                // This will send a redundant message if the same client is
                // handling multiple stations for a single unit, but it doesn't
                // matter
                for (const auto &stationPair : assignments[unitState.team][unitState.unit])
                {
                    EnvelopeMessage envelope(unitState, stationPair.second);
                    EventSystem::getGlobalInstance()->queueEvent(envelope);
                }
            }
        }

        // Deliver latest SonarDisplayState to every attached client
        for (const RakNet::RakNetGUID &client : all_clients)
        {
            EnvelopeMessage envelope(sonar, client);
            EventSystem::getGlobalInstance()->queueEvent(envelope);
        }
    }
}

inline bool didCollide(int64_t x1, int64_t y1, int64_t x2, int64_t y2, int32_t radius)
{
    return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) > radius * radius;
}

void SimulationMaster::runSimForUnit(UnitState *unitState)
{
    // Update submarine position
    unitState->x += unitState->speed * cos(unitState->heading * 2*M_PI/360.0);
    unitState->y += unitState->speed * sin(unitState->heading * 2*M_PI/360.0);

    // Check for collision with every torpedo
    std::vector<TorpedoID> torpedosHit;
    for (auto &torpedoPair : torpedos)
    {
        constexpr int collisionRadius = 10;
        if (didCollide(
                unitState->x, unitState->y,
                torpedoPair.second.x, torpedoPair.second.y,
                collisionRadius))
        {
            torpedosHit.push_back(torpedoPair.first);
        }
    }
    for (TorpedoID torpedoHit : torpedosHit)
    {
        torpedos.erase(torpedoHit);
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

    // Precalculate all_clients as the set of all distinct clients
    for (auto& teamPair : assignments) {
        for (auto &units : teamPair.second) {
            for (auto &stationPair : units) {
                all_clients.insert(stationPair.second);
            }
        }
    }

    // Initialize unitStates
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

