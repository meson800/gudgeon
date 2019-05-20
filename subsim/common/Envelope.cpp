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
                    source
                        >> us.team >> us.unit
                        >> us.tubeIsArmed >> us.tubeOccupancy >> us.remainingTorpedos >> us.remainingMines >> us.torpedoDistance
                        >> us.x >> us.y >> us.depth >> us.heading >> us.direction >> us.pitch
                        >> us.speed >> us.powerAvailable >> us.powerUsage
                        >> us.yawEnabled >> us.pitchEnabled >> us.engineEnabled >> us.commsEnabled >> us.sonarEnabled
                        >> us.weaponsEnabled;

                    EventSystem::getGlobalInstance()->queueEvent(us);
                }
                break;

                case Events::Sim::SonarDisplay:
                {
                    SonarDisplayState sd;
                    uint32_t len;
                    source >> len;
                    sd.dots.resize(len);
                    for (int i = 0; i < len; ++i) {
                      source >> sd.dots[i].x >> sd.dots[i].y;
                    }

                    EventSystem::getGlobalInstance()->queueEvent(sd);
                }
                break;

                case Events::Sim::Throttle:
                {
                    ThrottleEvent te;
                    source >> te.team >> te.unit >> te.speed;

                    EventSystem::getGlobalInstance()->queueEvent(te);
                }
                break;

                case Events::Sim::TubeLoad:
                {
                    TubeLoadEvent te;
                    source >> te.team >> te.unit >> te.tube >> te.type;

                    EventSystem::getGlobalInstance()->queueEvent(te);
                }
                break;

                case Events::Sim::TubeArm:
                {
                    TubeArmEvent te;
                    source >> te.team >> te.unit >> te.tube >> te.isArmed;

                    EventSystem::getGlobalInstance()->queueEvent(te);
                }
                break;

                case Events::Sim::Steering:
                {
                    SteeringEvent se;
                    source >> se.team >> se.unit >> se.tube >> se.direction >> se.isPressed;

                    EventSystem::getGlobalInstance()->queueEvent(se);
                }
                break;

                case Events::Sim::Fire:
                {
                    FireEvent fe;
                    source >> fe.team >> fe.unit;

                    EventSystem::getGlobalInstance()->queueEvent(fe);
                }
                break;

                case Events::Sim::Range:
                {
                    RangeEvent re;
                    source >> re.team >> re.unit >> re.range;

                    EventSystem::getGlobalInstance()->queueEvent(re);
                }
                break;

                case Events::Sim::Power:
                {
                    PowerEvent pe;
                    source >> pe.team >> pe.unit >> pe.system >> pe.isOn;

                    EventSystem::getGlobalInstance()->queueEvent(pe);
                }
                break;

                case Events::Sim::Sonar:
                {
                    SonarEvent se;
                    source >> se.team >> se.unit >> se.isActive;

                    EventSystem::getGlobalInstance()->queueEvent(se);
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

                    source
                        << us->team << us->unit
                        << us->tubeIsArmed << us->tubeOccupancy << us->remainingTorpedos << us->remainingMines << us->torpedoDistance
                        << us->x << us->y << us->depth << us->heading << us->direction << us->pitch 
                        << us->speed << us->powerAvailable << us->powerUsage
                        << us->yawEnabled << us->pitchEnabled << us->engineEnabled << us->commsEnabled << us->sonarEnabled
                        << us->weaponsEnabled;
                }
                break;

                case Events::Sim::SonarDisplay:
                {
                    SonarDisplayState* sd = (SonarDisplayState*)event.get();
                    uint32_t len = sd->dots.size();

                    source << len;
                    for (auto dot : sd->dots) {
                      source << dot.x << dot.y;
                    }
                }
                break;

                case Events::Sim::Throttle:
                {
                    ThrottleEvent* te = (ThrottleEvent*)event.get();

                    source << te->team << te->unit << te->speed;
                }
                break;

                case Events::Sim::TubeLoad:
                {
                    TubeLoadEvent* te = (TubeLoadEvent*)event.get();

                    source << te->team << te->unit << te->tube << te->type;
                }
                break;

                case Events::Sim::TubeArm:
                {
                    TubeArmEvent* te = (TubeArmEvent*)event.get();

                    source << te->team << te->unit << te->tube << te->isArmed;
                }
                break;

                case Events::Sim::Steering:
                {
                    SteeringEvent* se = (SteeringEvent*)event.get();
                    source << se->team << se->unit << se->tube << se->direction << se->isPressed;
                }
                break;

                case Events::Sim::Fire:
                {
                    FireEvent* fe = (FireEvent*)event.get();
                    source << fe->team << fe->unit;
                }
                break;

                case Events::Sim::Range:
                {
                    RangeEvent* re = (RangeEvent*)event.get();
                    source << re->team << re->unit << re->range;
                }
                break;

                case Events::Sim::Power:
                {
                    PowerEvent* pe = (PowerEvent*)event.get();
                    source << pe->team << pe->unit << pe->system << pe->isOn;
                }
                break;

                case Events::Sim::Sonar:
                {
                    SonarEvent* se = (SonarEvent*)event.get();
                    source << se->team << se->unit << se->isActive;
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
