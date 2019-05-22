#pragma once
namespace Events {

enum Category
{
    Invalid = 0,
    Network,
    UI,
    MockUI,
    Simulation,
};

enum Net
{
    Envelope
};

enum MockUIEvents
{
    Key,
    Text,
    IgnoreKeypresses,
    TeamOwnership
};

enum Sim
{
    SimStart,
    SimStartServer,
    UnitState,
    SonarDisplay,
    TextMessage,
    Throttle,
    TubeLoad,
    TubeArm,
    Steering,
    Fire,
    Range,
    Power,
    Stealth,
    Explosion,
    Config,
    Score,
    StatusUpdate,
};

} //namespace events
