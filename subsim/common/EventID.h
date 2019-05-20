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
};

enum Control
{
    Power,
    Weapon,
};
}
