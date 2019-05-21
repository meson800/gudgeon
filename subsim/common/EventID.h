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
    IgnoreKeypresses
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
    Sonar,
    TerrainData,
};

} //namespace events
