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
    Key
};

enum Sim
{
    SimStart,
};

enum Control
{
    Power,
    Weapon,
};
}
