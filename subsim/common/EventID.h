#pragma once
namespace Events {

enum Categories
{
    Invalid = 0,
    Netework,
    GUI
};

enum Network
{
    Connected
};

enum GUIEvents
{
    KeyDown,
    KeyUp
};
}
