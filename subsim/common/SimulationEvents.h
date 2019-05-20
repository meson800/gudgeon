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
