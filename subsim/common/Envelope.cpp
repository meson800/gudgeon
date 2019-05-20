#include "Messages.h"
#include "EventSystem.h"
#include "EventID.h"

#include "BitStreamHelper.h"

#include "Log.h"

#include <sstream>

// Headers with actual message types
#include "SimulationEvents.h"

/*!
 * Definition of an ostream override so that we can easily log
 * RakNetGUID's
 */
static std::ostream& operator<< (std::ostream& stream, const RakNet::RakNetGUID& guid)
{
    return stream << RakNet::RakNetGUID::ToUint32(guid);
}

RakNet::MessageID EnvelopeMessage::getType() const
{
    return MessageType::ID_ENVELOPE;
}


void EnvelopeMessage::deserialize(RakNet::BitStream& source)
{
    uint32_t category;
    uint32_t id;

    source >> category >> id;

    switch (category)
    {
        case Events::Category::Simulation:
        {
            switch (id)
            {
                case Events::Sim::SimStart:
                {
                    SimulationStart simevent;
                    uint32_t len;
                    source >> len;

                    std::ostringstream sstream;
                    for (uint32_t i = 0; i < len; ++i)
                    {
                        SimulationStart::Station station;
                        source >> station.team >> station.unit >> station.station;

                        sstream << "(" << station.team << "," << station.unit << "," << StationNames[station.station] << ")";

                        simevent.stations.push_back(station);
                    }

                    Log::writeToLog(Log::L_DEBUG, "Deserialized a SimStart event from node ", address, ". Responsible for stations: ", sstream.str());

                        

                    // inject event
                    EventSystem::getGlobalInstance()->queueEvent(simevent);
                }
                break;

                case Events::Sim::UnitState:
                {
                    UnitState us;
                    source >> us.tubeIsArmed >> us.tubeOccupancy >> us.torpedoDistance
                        >> us.x >> us.y >> us.depth >> us.heading >> us.pitch
                        >> us.speed >> us.powerAvailable >> us.powerUsage
                        >> us.yawEnabled >> us.pitchEnabled >> us.engineEnabled >> us.commsEnabled >> us.sonarEnabled
                        >> us.weaponsEnabled;

                    EventSystem::getGlobalInstance()->queueEvent(us);
                }
                break;

                default:
                Log::writeToLog(Log::ERR, "Attempted to deserialize a simulation event of id=", id, " from an envelope without deserialization code defined!");
                throw EnvelopeError("Cannot deserialize a simulation event from an envelope.");
            }
        }
        break;

        default:
        Log::writeToLog(Log::ERR, "Attempted to deserialize an event of category=", category, " and id=", id, " from an envelope, but no serialization code defined!");
        throw EnvelopeError("Cannot deserialize an event into an envelope");
        break;
    }
}

void EnvelopeMessage::serialize(RakNet::BitStream& source) const
{
    if (!event)
    {
        Log::writeToLog(Log::ERR, "Attempted to wrap an empty event in an envelope!");
        throw EnvelopeError("Cannot serialize an empty envelope.");
    }
    uint32_t category = event->i_category;
    uint32_t id = event->i_id;

    source << category << id;

    switch (category)
    {
        case Events::Category::Simulation:
        {
            switch (id)
            {
                case Events::Sim::SimStart:
                {
                    SimulationStart* simevent = (SimulationStart*)event.get();
                    uint32_t len = simevent->stations.size();

                    source << len;
                    for (auto station : simevent->stations)
                    {
                        source << station.team << station.unit << station.station;
                    }
                }
                break;

                case Events::Sim::UnitState:
                {
                    UnitState* us = (UnitState*)event.get();

                    source << us->tubeIsArmed << us->tubeOccupancy << us->torpedoDistance
                        << us->x << us->y << us->depth << us->heading << us->pitch 
                        << us->speed << us->powerAvailable << us->powerUsage
                        << us->yawEnabled << us->pitchEnabled << us->engineEnabled << us->commsEnabled << us->sonarEnabled
                        << us->weaponsEnabled;
                }
                break;

                default:
                Log::writeToLog(Log::ERR, "Attempted to wrap a simulation event of id=", id, " in an envelope without serialization code defined!");
                throw EnvelopeError("Cannot serialize a simulation event into an envelope.");
            }
        }
        break;

        default:
        Log::writeToLog(Log::ERR, "Attempted to wrap an event of category=", category, " and id=", id, " in an envelope, but no serialization code defined!");
        throw EnvelopeError("Cannot serialize an event into an envelope");
        break;
    }
}

EnvelopeMessage::EnvelopeMessage(RakNet::BitStream& source, RakNet::RakNetGUID address_)
    : address(address_)
    , Event(category, type)
{
    deserialize(source);
}
