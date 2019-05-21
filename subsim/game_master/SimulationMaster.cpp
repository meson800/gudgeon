#include "SimulationMaster.h"
#include <sstream>
#include <math.h>

#include "../common/Log.h"
#include "../common/Exceptions.h"


SimulationMaster::SimulationMaster(Network* network_, const std::string& filename)
    : shouldShutdown(false)
    , network(network_)
    , EventReceiver({
        dispatchEvent<SimulationMaster, SimulationStartServer, &SimulationMaster::simStart>,
        dispatchEvent<SimulationMaster, ThrottleEvent, &SimulationMaster::throttle>,
        dispatchEvent<SimulationMaster, SteeringEvent, &SimulationMaster::steering>,
        dispatchEvent<SimulationMaster, FireEvent, &SimulationMaster::fire>,
        dispatchEvent<SimulationMaster, TubeLoadEvent, &SimulationMaster::tubeLoad>,
        dispatchEvent<SimulationMaster, TubeArmEvent, &SimulationMaster::tubeArm>,
        dispatchEvent<SimulationMaster, PowerEvent, &SimulationMaster::power>,
    })
{
    ParseResult result = GenericParser::parse(filename);
    config = ConfigParser::parseConfig(result);

    
    lobbyInit = std::unique_ptr<LobbyHandler>(new LobbyHandler(result));
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
            torpedo->x += config.torpedoSpeed * cos(torpedo->heading * 2*M_PI/360.0);
            torpedo->y += config.torpedoSpeed * sin(torpedo->heading * 2*M_PI/360.0);
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
                unitSonarState.heading = unitState.heading;
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
    // Update submarine heading
    if (unitState->direction == UnitState::SteeringDirection::Right)
    {
        int32_t newHeading = static_cast<int32_t>(unitState->heading) - config.subTurningSpeed;
        unitState->heading = newHeading < 0 ? newHeading + 360 : newHeading;
    }

    if (unitState->direction == UnitState::SteeringDirection::Left)
    {
        int32_t newHeading = static_cast<int32_t>(unitState->heading) + config.subTurningSpeed;
        unitState->heading = newHeading > 360 ? newHeading - 360 : newHeading;
    }

    // Update submarine position
    int64_t nextX = unitState->x + unitState->speed * cos(unitState->heading * 2*M_PI/360.0);
    int64_t nextY = unitState->y + unitState->speed * sin(unitState->heading * 2*M_PI/360.0);

    // Check for collision versus terrain
    int64_t scaledX = nextX / config.terrain.scale;
    int64_t scaledY = nextY / config.terrain.scale;


    if (config.terrain.colorAt(scaledX, scaledY) == Terrain::WALL)
    {
        // Terrain collision!
        unitState->speed = 0;
    } else {
        unitState->x = nextX;
        unitState->y = nextY;
    }
        
    // Check for collision with every torpedo
    std::vector<TorpedoID> torpedosHit;
    for (auto &torpedoPair : torpedos)
    {
        if (didCollide(
                unitState->x, unitState->y,
                torpedoPair.second.x, torpedoPair.second.y,
                config.collisionRadius))
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

        // Make sure we have a starting position
        if (config.startLocations[teamPair.first].size() == 0)
        {
            Log::writeToLog(Log::ERR, "Team ", teamPair.first, " had no starting position in the map!");
            throw ConfigParseError("Not enough start positions defined");
        }

        std::pair<int64_t, int64_t> start = config.startLocations[teamPair.first][0];
        start.first *= config.terrain.scale;
        start.first += config.terrain.scale / 2;
        start.second *= config.terrain.scale;
        start.second += config.terrain.scale / 2;

        for (uint32_t unit = 0; unit < teamPair.second.size(); ++unit) {
            UnitState unitState;
            unitState.team = teamPair.first;
            unitState.unit = unit;
            unitState.tubeIsArmed = std::vector<bool>(5, false);
            unitState.tubeOccupancy = std::vector<UnitState::TubeStatus>(5, UnitState::Empty);
            unitState.remainingTorpedos = config.maxTorpedos;
            unitState.remainingMines = config.maxMines;
            unitState.torpedoDistance = 100;
            unitState.x = start.first
                + config.collisionRadius * 2
                    * (unit - 0.5 * (teamPair.second.size() - 1));
            unitState.y = start.second;
            unitState.depth = 0;
            unitState.heading = 90;
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

    // Send config data to all connected clients
    ConfigEvent configEvent;
    configEvent.config = config;

    for (auto& client : all_clients)
    {
        EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(configEvent, client));
    }


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
        unitStates[event->team][event->unit].speed =
            event->speed > config.subMaxSpeed ? config.subMaxSpeed : event->speed;
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
                torp.x = unit.x + 1.5 * config.collisionRadius * cos(unit.heading * 2*M_PI/360.0);
                torp.y = unit.y + 1.5 * config.collisionRadius * sin(unit.heading * 2*M_PI/360.0);
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

        if (unit.tubeIsArmed[event->tube] == false)
        {
            // Give a credit if there is already a loaded weapon in the tube
            if (unit.tubeOccupancy[event->tube] == UnitState::TubeStatus::Torpedo)
            {
                ++unit.remainingTorpedos;
            }

            if (unit.tubeOccupancy[event->tube] == UnitState::TubeStatus::Mine)
            {
                ++unit.remainingMines;
            }

            // Reload the tube
            if (event->type == TubeLoadEvent::AmmoType::Torpedo && unit.remainingTorpedos > 0)
            {
                unit.tubeOccupancy[event->tube] = UnitState::TubeStatus::Torpedo;
                --unit.remainingTorpedos;
            } else if (event->type == TubeLoadEvent::AmmoType::Mine && unit.remainingMines > 0) {
                unit.tubeOccupancy[event->tube] = UnitState::TubeStatus::Mine;
                --unit.remainingMines;
            }
        }
    }
    return HandleResult::Stop;
}

HandleResult SimulationMaster::power(PowerEvent* event)
{
    {
        std::lock_guard<std::mutex> lock(stateMux);

        UnitState& unit = unitStates[event->team][event->unit];

        switch (event->system)
        {
            case PowerEvent::System::Yaw:
                unit.yawEnabled = event->isOn;
            break;

            case PowerEvent::System::Pitch:
                unit.pitchEnabled = event->isOn;
            break;

            case PowerEvent::System::Engine:
                unit.engineEnabled = event->isOn;
            break;

            case PowerEvent::System::Comms:
                unit.commsEnabled = event->isOn;
            break;

            case PowerEvent::System::Sonar:
                unit.sonarEnabled = event->isOn;
            break;

            case PowerEvent::System::Weapons:
                unit.weaponsEnabled = event->isOn;
            break;
            
            default:
            break;
        }
    }
    return HandleResult::Stop;
}
