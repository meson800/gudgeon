#pragma once

#include <map>
#include <vector>

#include "EventID.h"
#include "Stations.h"
#include "EventSystem.h"

#include "RakNetTypes.h" // For RakNetGUID

namespace RakNet
{
    /// Forward declaration
    class BitStream;
}

/*!
 *
 * Event delivered to systems when the simulation starts. Each client gets sent this message,
 * which stores which stations this client is responsible for, and the team/unit.
 */
class SimulationStart : public Event
{
public:
    SimulationStart() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::SimStart;

    struct Station
    {
        uint32_t team;
        uint32_t unit;
        StationType station;
    };

    std::vector<Station> stations;
};

/*!
 * Event delivered by the game master to itself when the lobby is full.
 */
class SimulationStartServer : public Event
{
public:
    SimulationStartServer() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::SimStart;


    std::map<uint32_t, std::vector<std::vector<std::pair<StationType, RakNet::RakNetGUID>>>> assignments;
};

/*!
 * Mega-event that reports a unit state back.
 *
 * This is everything the client needs to display/interact with.
 */
class UnitSimulationState : public Event
{
public:
    UnitSimulationState() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::UnitState;
};

/*!
 * Event that stores a text message sent by one of the teams.
 */
class TextMessage : public Event
{
public:
    TextMessage() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::TextMessage;

    std::string message;
};

/*!
 * Event that stores the complete state of a Unit. The game master sends
 * these messages out to the clients after every simulation time step.
 */
class UnitState : public Event
{
public:
    UnitState() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::UnitState;

    /**
     * Stores the armed/reload status of the tubes.
     * Tubes can only be reloaded if they are not armed, and
     * tubes can only be fired if they are armed
     */
    std::vector<bool> tubeIsArmed;

    /// Stores the different possible loaded states
    enum TubeStatus
    {
        Empty,
        Torpedo,
        Mine
    };

    /// Stores the occupancy of the tubes
    std::vector<TubeStatus> tubeOccupancy;

    /// Number Torpedos/Mines remaining
    uint16_t remainingTorpedos;
    uint16_t remainingMines;

    /// Stores the current maximum distance of the torpedos
    uint64_t torpedoDistance;

    /// Stores the position of the sub. These are int's for easy serialization purposes
    int64_t x, y, depth;

    /// Stores the current heading of the sub in degrees. This is also an integer for easy serialization
    uint16_t heading;

    /// Stores the current pitch of the sub, also in degrees
    int16_t pitch;

    /// Stores the current speed of the sub, as this affects power usage.
    uint16_t speed;

    /// Stores the current power available (used as health)
    uint16_t powerAvailable;

    /// Stores the current power usage
    uint16_t powerUsage;

    /// Stores the enable/disable status of various power items
    bool yawEnabled, pitchEnabled, engineEnabled, commsEnabled, sonarEnabled, weaponsEnabled;
};
