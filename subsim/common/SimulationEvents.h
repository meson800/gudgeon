#pragma once

#include <map>
#include <vector>

#include "EventID.h"
#include "Stations.h"
#include "EventSystem.h"

#include "RakNetTypes.h" // For RakNetGUID

#include "ConfigParser.h" // for Terrain

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

    uint32_t team;
    uint32_t unit;

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

    /// Stores the different steering directions
    enum SteeringDirection
    {
        Left,
        Right,
        Center,
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

    /// Stores the current steering direction
    SteeringDirection direction;


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
 * Stores the subset of UnitState that should be visible to other units on sonar
 */
struct UnitSonarState
{
    uint32_t team;
    uint32_t unit;

    int64_t x, y, depth;
    uint16_t heading;
};

/*!
 * Event that stores the complete state of every visible entity; essentially, a
 * list of all the dots that should be displayed on the sonar screen.
 */
class SonarDisplayState : public Event
{
public:
    SonarDisplayState() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::SonarDisplay;

    std::vector<UnitSonarState> units;
    std::vector<TorpedoState> torpedos;
    std::vector<MineState> mines;
};

class ThrottleEvent : public Event
{
public:
    ThrottleEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::Throttle;

    uint32_t team;
    uint32_t unit;
    uint16_t speed;
};

/*!
 * Stores when a tube is loaded with a torpedo or a mine
 */
class TubeLoadEvent : public Event
{
public:

    TubeLoadEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::TubeLoad;

    enum AmmoType
    {
        Torpedo,
        Mine
    };

    uint32_t team;
    uint32_t unit;
    uint16_t tube;
    AmmoType type;
};

/*!
 * Stores when a tube is armed/safed. You can only fire when armed
 * and you can only reload when safed.
 */
class TubeArmEvent : public Event
{
public:
    TubeArmEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::TubeArm;

    uint32_t team;
    uint32_t unit;
    uint16_t tube;
    bool isArmed;
};

/*!
 * Stores when a steering command is inputted. Currently only handles left/right steering.
 */
class SteeringEvent : public Event
{
public:
    SteeringEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::Steering;

    enum Direction
    {
        Left,
        Right
    };

    uint32_t team;
    uint32_t unit;
    Direction direction;
    bool isPressed;
};

/*! 
 * Stores when the fire button is pressed.
 */
class FireEvent : public Event
{
public:
    FireEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::Fire;

    uint32_t team;
    uint32_t unit;
};

/*! 
 * Stores when the torpedo range has been updated.
 */
class RangeEvent : public Event
{
public:
    RangeEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::Range;

    uint32_t team;
    uint32_t unit;
    uint16_t range;
};

/*!
 * Stores when the power state of a system has been updated.
 */
class PowerEvent : public Event
{
public:
    PowerEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::Power;

    enum System
    {
        Yaw,
        Pitch,
        Engine,
        Comms,
        Sonar,
        Weapons
    };
        
    uint32_t team;
    uint32_t unit;
    System system;
    bool isOn;
};

/*!
 * Stores if the sonar is in active or passive mode
 */
class SonarEvent : public Event
{
public:
    SonarEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::Sonar;

    uint32_t team;
    uint32_t unit;
    bool isActive;
};

/*!
 * Sends terrain data to the client
 */
class ConfigEvent : public Event
{
public:
    ConfigEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::Simulation;
    constexpr static uint32_t id = Events::Sim::Config;

    Config config;
};
