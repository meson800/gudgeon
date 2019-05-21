#include "SimulationMaster.h"
#include <sstream>
#include <math.h>

#include "../common/Log.h"

constexpr static int32_t STEERING_RATE = 2;

SimulationMaster::SimulationMaster(Network* network_)
    : shouldShutdown(false)
    , network(network_)
    , EventReceiver({
        dispatchEvent<SimulationMaster, SimulationStartServer, &SimulationMaster::simStart>,
        dispatchEvent<SimulationMaster, ThrottleEvent, &SimulationMaster::throttle>,
        dispatchEvent<SimulationMaster, SteeringEvent, &SimulationMaster::steering>,
        dispatchEvent<SimulationMaster, FireEvent, &SimulationMaster::fire>,
        dispatchEvent<SimulationMaster, TubeLoadEvent, &SimulationMaster::tubeLoad>,
        dispatchEvent<SimulationMaster, TubeArmEvent, &SimulationMaster::tubeArm>,
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
            TorpedoState *torpedo = &torpedoPair.second;
            constexpr int torpedo_speed = 10;
            torpedo->x += torpedo_speed * cos(torpedo->heading * 2*M_PI/360.0);
            torpedo->y += torpedo_speed * sin(torpedo->heading * 2*M_PI/360.0);
            sonar.torpedos.push_back(*torpedo);
        }

        for (auto& minePair : mines)
        {
            sonar.mines.push_back(minePair.second);
        }

        for (auto& teamPair : unitStates)
        {
            for (UnitState &unitState : teamPair.second)
            {
                runSimForUnit(&unitState);

                UnitSonarState unitSonarState;
                unitSonarState.team = unitState.team;
                unitSonarState.unit = unitState.unit;
                unitSonarState.x = unitState.x;
                unitSonarState.y = unitState.y;
                unitSonarState.depth = unitState.depth;
                sonar.units.push_back(unitSonarState);

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
    return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) < radius * radius;
}

void SimulationMaster::runSimForUnit(UnitState *unitState)
{
    // Update submarine heading, only if we are moving
    if (unitState->speed > 0)
    {
        if (unitState->direction == UnitState::SteeringDirection::Left)
        {
            int32_t newHeading = static_cast<int32_t>(unitState->heading) - STEERING_RATE;
            unitState->heading = newHeading < 0 ? newHeading + 360 : newHeading;
        }

        if (unitState->direction == UnitState::SteeringDirection::Right)
        {
            int32_t newHeading = static_cast<int32_t>(unitState->heading) + STEERING_RATE;
            unitState->heading = newHeading > 360 ? newHeading - 360 : newHeading;
        }
    }

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
            Log::writeToLog(Log::INFO, "Torpedo struck submarine");
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
            unitState.direction = UnitState::SteeringDirection::Center;
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

    nextTorpedoID = nextMineID = 1;

    TorpedoState torp;
    torp.x = 80;
    torp.y = 80;
    torp.depth = 0;
    torp.heading = 0;
    torpedos[nextTorpedoID++] = torp;

    MineState mine;
    mine.x = 200;
    mine.y = 200;
    mine.depth = 0;
    mines[nextMineID++] = mine;

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


HandleResult SimulationMaster::steering(SteeringEvent *event)
{
    {
        std::lock_guard<std::mutex> lock(stateMux);
        if (event->isPressed == false)
        {
            unitStates[event->team][event->unit].direction = UnitState::SteeringDirection::Center;
        } else if (event->direction == SteeringEvent::Direction::Left) {
            unitStates[event->team][event->unit].direction = UnitState::SteeringDirection::Left;
        } else if (event->direction == SteeringEvent::Direction::Right) {
            unitStates[event->team][event->unit].direction = UnitState::SteeringDirection::Right;
        }
    }
    return HandleResult::Stop;
}

/// Handles the event when the submarine fires its armed torpedos/mines
HandleResult SimulationMaster::fire(FireEvent *event)
{
    {
        std::lock_guard<std::mutex> lock(stateMux);
        UnitState& unit = unitStates[event->team][event->unit];

        for (int i = 0; i < unit.tubeOccupancy.size(); ++i)
        {
            if (unit.tubeIsArmed[i]
                && unit.tubeOccupancy[i] == UnitState::TubeStatus::Torpedo)
            {
                TorpedoState torp;
                torp.x = unit.x + 30 * cos(unit.heading * 2*M_PI/360.0);
                torp.y = unit.y + 30 * sin(unit.heading * 2*M_PI/360.0);
                torp.depth = unit.depth;
                torp.heading = unit.heading;
                torpedos[nextTorpedoID++] = torp;

                Log::writeToLog(Log::L_DEBUG, "Fired torpedo from team ",
                    unit.team, " unit ", unit.unit, " from tube ", i);

                unit.tubeOccupancy[i] = UnitState::TubeStatus::Empty;
            }

            if (unit.tubeIsArmed[i]
                && unit.tubeOccupancy[i] == UnitState::TubeStatus::Mine)
            {
                Log::writeToLog(Log::L_DEBUG, "Laid mine from team ",
                    unit.team, " unit ", unit.unit, " from tube ", i);
                unit.tubeOccupancy[i] = UnitState::TubeStatus::Empty;
            }
        }
    }
    return HandleResult::Stop;
}

HandleResult SimulationMaster::tubeArm(TubeArmEvent *event)
{
    {
        std::lock_guard<std::mutex> lock(stateMux);
        unitStates[event->team][event->unit].tubeIsArmed[event->tube] = event->isArmed;
        Log::writeToLog(Log::L_DEBUG, "Team ", event->team, " unit ", event->unit,
            event->isArmed ? " armed tube " : " disarmed tube ", event->tube);
    }
    return HandleResult::Stop;

}

HandleResult SimulationMaster::tubeLoad(TubeLoadEvent *event)
{
    {
        std::lock_guard<std::mutex> lock(stateMux);

        UnitState & unit = unitStates[event->team][event->unit];

        if (unit.tubeIsArmed[event->tube] == false
            && unit.tubeOccupancy[event->tube] == UnitState::TubeStatus::Empty)
        {
            if (event->type == TubeLoadEvent::AmmoType::Torpedo)
            {
                unit.tubeOccupancy[event->tube] = UnitState::TubeStatus::Torpedo;
            } else if (event->type == TubeLoadEvent::AmmoType::Mine) {
                unit.tubeOccupancy[event->tube] = UnitState::TubeStatus::Mine;
            }
        }
    }
    return HandleResult::Stop;
}
