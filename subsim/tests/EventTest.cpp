#include "../common/EventSystem.h"

#include <iostream>

class EventTest : public Event
{
public:
    EventTest() : Event(category, id) {}
    constexpr static uint32_t category = 1;
    constexpr static uint32_t id = 1;
};

class EventHandler
{
public:
    HandleResult handleTest(EventTest* test)
    {
        std::cout << "Handled test event\n";
        return HandleResult::Stop;
    }
};

int main()
{
    EventHandler handler;
    EventTest test;
    dispatchEvent<EventHandler, EventTest, &EventHandler::handleTest>(&handler, &test);
}
