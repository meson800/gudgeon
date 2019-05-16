#pragma once

#include "EventID.h"
#include "Stations.h"

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

