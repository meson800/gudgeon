Making new events
=================
1. Pick an ID category and an ID number for your new event, adding
   it to common/EventID.h. Create a new enum if needed.
2. Define your new event somewhere, inheriting from Event. Importantly,
   add constexpr static members for your category and number; the template
   system needs it. The bare minimum is therefore:

class TestEvent : public Event
{
public:
    TestEvent() : Event(category, id) {}
    constexpr static uint32_t category = 1;
    constexpr static uint32_t id = 1;
};


Receiving events
================
1. Inherit from EventReceiver. In your constructor, initalize it with dispatchEvent, with the list of events you want to capture.

    : EventReceiver(dispatchEvent<TestHandler, EventTest, &TestHandler::handleTest>)

    The first template parameter is the name of your class, then followed by pairs of events to capture along with function signatures
    that take a pointer to the event type, and returns a HandleResult

