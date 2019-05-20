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
    TextMessage,
    Throttle,
};

enum Control
{
    Power,
    Weapon,
};
}
