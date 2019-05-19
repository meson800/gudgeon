#include "../common/EventSystem.h"
#include "../common/EventID.h"

enum Key
{
    Other,
    Letter,
    Up,
    Down,
    Left,
    Right,
    Enter
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
