#include "../common/Log.h"
#include "../common/EventSystem.h"

#include <iostream>

int result = 1;

class EventTest : public Event
{
public:
    EventTest() : Event(category, id) {}
    constexpr static uint32_t category = 1;
    constexpr static uint32_t id = 1;
};

class TestHandler : public EventReceiver
{
public:
    TestHandler()
        : EventReceiver(dispatchEvent<TestHandler, EventTest, &TestHandler::handleTest>)
    {}

    HandleResult handleTest(EventTest* test)
    {
        std::cout << "TEST SUCCESS: Handled test event\n";
        result = 0;
        return HandleResult::Stop;
    }

};

int main(int argc, char **argv)
{
    Log::setLogfile(std::string(argv[0]) + ".log");
    Log::clearLog();
    Log::shouldMirrorToConsole(true);
    Log::setLogLevel(Log::ALL);

    TestHandler handler;
    EventTest test;
    EventSystem system;
    system.registerCallback(&handler);
    system.queueEvent(test);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return result;
}
