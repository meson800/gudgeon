#pragma once

#include "../common/EventSystem.h"
#include "../common/EventID.h"

#include <string>

enum Key
{
    Other,
    Letter,
    Up,
    Down,
    Left,
    Right,
    Enter,
    LBracket,
    RBracket,
    Backslash,
    Space,
    One,
    Two,
    Three,
    Four,
    Five,
    Q,
    W,
    E,
    R,
    T,
    A,
    S,
    D,
    F,
    G,
    H,
    J,
    K,
    L,
};

class KeyEvent : public Event
{
public:
    KeyEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::MockUI;
    constexpr static uint32_t id = Events::MockUIEvents::Key;

    Key key;
    char letter;
    bool isDown;
    uint8_t repeat;
};

/**
 * Stores an edited version of text, as derived from SDL.
 */
class TextInputEvent : public Event
{
public:
    TextInputEvent() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::MockUI;
    constexpr static uint32_t id = Events::MockUIEvents::Text;

    std::string text;
};

/** 
 * Event sent by "shitty IRC", letting other modules know to ignore/not ignore
 * key events and text input events, as we are capturing them.
 */
class IgnoreKeypresses : public Event
{
public:
    IgnoreKeypresses() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::MockUI;
    constexpr static uint32_t id = Events::MockUIEvents::IgnoreKeypresses;

    bool shouldIgnore;
};

/**
 * Event sent when we know what team we are on
 */
class TeamOwnership : public Event
{
public:
    TeamOwnership() : Event(category, id) {}
    constexpr static uint32_t category = Events::Category::MockUI;
    constexpr static uint32_t id = Events::MockUIEvents::TeamOwnership;

    uint32_t team;
};
