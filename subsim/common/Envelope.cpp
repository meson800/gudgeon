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
                        >> us.speed >> us.desiredSpeed
                        >> us.powerAvailable >> us.powerUsage >> us.isStealth >> us.stealthCooldown
                        >> us.respawning >> us.respawnCooldown
                        >> us.yawEnabled >> us.pitchEnabled >> us.engineEnabled >> us.commsEnabled >> us.sonarEnabled
                        >> us.weaponsEnabled
                        >> us.targetIsLocked >> us.targetTeam >> us.targetUnit
                        >> us.hasFlag >> us.flag.team >> us.flag.index;

                    EventSystem::getGlobalInstance()->queueEvent(us);
                }
                break;

                case Events::Sim::SonarDisplay:
                {
                    SonarDisplayState sd;
                    uint32_t len;

                    source >> len;
                    sd.units.resize(len);
                    for (int i = 0; i < len; ++i)
                    {
                        source >> sd.units[i].team >> sd.units[i].unit
                            >> sd.units[i].x >> sd.units[i].y >> sd.units[i].depth
                            >> sd.units[i].heading >> sd.units[i].speed >> sd.units[i].power
                            >> sd.units[i].hasFlag
                            >> sd.units[i].isStealth >> sd.units[i].stealthCooldown
                            >> sd.units[i].respawning >> sd.units[i].respawnCooldown;
                    }

                    source >> len;
                    sd.torpedos.resize(len);
                    for (int i = 0; i < len; ++i)
                    {
                        source >> sd.torpedos[i].x >> sd.torpedos[i].y >> sd.torpedos[i].depth
                            >> sd.torpedos[i].heading;
                    }

                    source >> len;
                    sd.mines.resize(len);
                    for (int i = 0; i < len; ++i)
                    {
                        source >> sd.mines[i].x >> sd.mines[i].y >> sd.mines[i].depth;
                    }

                    source >> len;
                    sd.flags.resize(len);
                    for (int i = 0; i < len; ++i)
                    {
                        source >> sd.flags[i].team >> sd.flags[i].x >> sd.flags[i].y
                            >> sd.flags[i].depth >> sd.flags[i].isTaken;
                    }

                    EventSystem::getGlobalInstance()->queueEvent(sd);
                }
                break;

                case Events::Sim::Throttle:
                {
                    ThrottleEvent te;
                    source >> te.team >> te.unit >> te.desiredSpeed;

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
                    source >> se.team >> se.unit >> se.direction >> se.isPressed;

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

                case Events::Sim::Stealth:
                {
                    StealthEvent se;
                    source >> se.team >> se.unit >> se.isStealth;

                    EventSystem::getGlobalInstance()->queueEvent(se);
                }
                break;

                case Events::Sim::Explosion:
                {
                    ExplosionEvent ee;
                    source >> ee.x >> ee.y >> ee.size;

                    EventSystem::getGlobalInstance()->queueEvent(ee);
                }
                break;

                case Events::Sim::Config:
                {
                    ConfigEvent ce;
                    source
                        >> ce.config.terrain.map
                        >> ce.config.terrain.width
                        >> ce.config.terrain.height
                        >> ce.config.terrain.scale
                        >> ce.config.startLocations
                        >> ce.config.flags
                        >> ce.config.subTurningSpeed
                        >> ce.config.subAcceleration
                        >> ce.config.subMaxSpeed 
                        >> ce.config.stealthSpeedLimit
                        >> ce.config.maxTorpedos
                        >> ce.config.maxMines
                        >> ce.config.sonarRange
                        >> ce.config.passiveSonarNoiseFloor
                        >> ce.config.torpedoSpread
                        >> ce.config.torpedoSpeed
                        >> ce.config.collisionRadius
                        >> ce.config.torpedoDamage
                        >> ce.config.mineDamage
                        >> ce.config.collisionDamage
                        >> ce.config.mineExclusionRadius
                        >> ce.config.frameMilliseconds
                        >> ce.config.stealthCooldown
                        >> ce.config.respawnCooldown;

                    EventSystem::getGlobalInstance()->queueEvent(ce);
                }
                break;

                case Events::Sim::Score:
                {
                    ScoreEvent se;
                    source >> se.scores;

                    EventSystem::getGlobalInstance()->queueEvent(se);
                }
                break;

                case Events::Sim::StatusUpdate:
                {
                    StatusUpdateEvent se;
                    source >> se.team >> se.unit >> se.type;

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
                        << us->speed << us->desiredSpeed
                        << us->powerAvailable << us->powerUsage << us->isStealth << us->stealthCooldown
                        << us->respawning << us->respawnCooldown
                        << us->yawEnabled << us->pitchEnabled << us->engineEnabled << us->commsEnabled << us->sonarEnabled
                        << us->weaponsEnabled
                        << us->targetIsLocked << us->targetTeam << us->targetUnit
                        << us->hasFlag << us->flag.team << us->flag.index;
                }
                break;

                case Events::Sim::SonarDisplay:
                {
                    SonarDisplayState* sd = (SonarDisplayState*)event.get();
                    uint32_t len;

                    len = sd->units.size();
                    source << len;
                    for (auto unit : sd->units)
                    {
                        source << unit.team << unit.unit
                          << unit.x << unit.y << unit.depth
                          << unit.heading << unit.speed << unit.power
                          << unit.hasFlag
                          << unit.isStealth << unit.stealthCooldown
                          << unit.respawning << unit.respawnCooldown;
                    }

                    len = sd->torpedos.size();
                    source << len;
                    for (auto torp : sd->torpedos)
                    {
                        source << torp.x << torp.y << torp.depth << torp.heading;
                    }

                    len = sd->mines.size();
                    source << len;
                    for (auto mine : sd->mines)
                    {
                        source << mine.x << mine.y << mine.depth;
                    }

                    len = sd->flags.size();
                    source << len;
                    for (auto flag : sd->flags)
                    {
                        source << flag.team << flag.x << flag.y << flag.depth << flag.isTaken;
                    }
                }
                break;

                case Events::Sim::Throttle:
                {
                    ThrottleEvent* te = (ThrottleEvent*)event.get();

                    source << te->team << te->unit << te->desiredSpeed;
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
                    source << se->team << se->unit << se->direction << se->isPressed;
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

                case Events::Sim::Stealth:
                {
                    StealthEvent* se = (StealthEvent*)event.get();
                    source << se->team << se->unit << se->isStealth;
                }
                break;

                case Events::Sim::Explosion:
                {
                    ExplosionEvent* ee = (ExplosionEvent*)event.get();
                    source << ee->x << ee->y << ee->size;
                }
                break;

                case Events::Sim::Config:
                {
                    ConfigEvent* ce = (ConfigEvent*)event.get();
                    source
                        << ce->config.terrain.map
                        << ce->config.terrain.width
                        << ce->config.terrain.height
                        << ce->config.terrain.scale
                        << ce->config.startLocations
                        << ce->config.flags
                        << ce->config.subTurningSpeed
                        << ce->config.subAcceleration
                        << ce->config.subMaxSpeed 
                        << ce->config.stealthSpeedLimit
                        << ce->config.maxTorpedos
                        << ce->config.maxMines
                        << ce->config.sonarRange
                        << ce->config.passiveSonarNoiseFloor
                        << ce->config.torpedoSpread
                        << ce->config.torpedoSpeed
                        << ce->config.collisionRadius
                        << ce->config.torpedoDamage
                        << ce->config.mineDamage
                        << ce->config.collisionDamage
                        << ce->config.mineExclusionRadius
                        << ce->config.frameMilliseconds
                        << ce->config.stealthCooldown
                        << ce->config.respawnCooldown;
                }
                break;

                case Events::Sim::Score:
                {
                    ScoreEvent* se = (ScoreEvent*)event.get();
                    source << se->scores;
                }
                break;

                case Events::Sim::StatusUpdate:
                {
                    StatusUpdateEvent* se = (StatusUpdateEvent*)event.get();
                    source << se->team << se->unit << se->type;
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
