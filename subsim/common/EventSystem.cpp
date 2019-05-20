#include "EventSystem.h"

#include "Exceptions.h"
#include "Log.h"

#include "Network.h"
#include "EventID.h"
#include "Messages.h"


Event::Event(uint32_t category_, uint32_t id_)
    : i_category(category_)
    , i_id(id_)
{}
Event::~Event() {}


EventReceiver::EventReceiver(std::vector<HandleResult(*)(EventReceiver*,Event*)> dispatchers_)
    : dispatchers(dispatchers_)
{
    EventSystem::getGlobalInstance()->registerCallback(this);
}

    /// Removes this class from the event system
EventReceiver::~EventReceiver()
{
    EventSystem::getGlobalInstance()->deregisterCallback(this);
}

EventSystem* EventSystem::singleton = nullptr;

EventSystem* EventSystem::getGlobalInstance()
{
    if (singleton == nullptr)
    {
        Log::writeToLog(Log::ERR, "Attempted to get event system singleton before it was setup!");
        throw EventError("Attempted to get invalid event singleton!");
    }

    return singleton;
}

void EventSystem::setGlobalInstance(EventSystem* system)
{
    singleton = system;
}

EventSystem::EventSystem(Network* network_)
    : shutdownFlag(false)
    , network(network_)
{
    if (singleton != nullptr)
    {
        Log::writeToLog(Log::ERR, "Event system singleton already set to ", singleton, " when a new EventSystem was created!");
        throw EventError("Attempted to set a new event system singleton while one was already assigned!");
    }
    setGlobalInstance(this);

    Log::writeToLog(Log::INFO, "Starting EventSystem delivery thread...");
    deliveryThread = std::thread(&EventSystem::deliverEvents, this);
}

EventSystem::~EventSystem()
{
    Log::writeToLog(Log::INFO, "Shutting down EventSystem delivery thread...");
    shutdownFlag = true;
    deliveryThread.join();
    Log::writeToLog(Log::INFO, "EventSystem delivery thread shutdown successful.");

    if (singleton != this)
    {
        Log::writeToLog(Log::ERR, "Attempt to deregister EventSystem:", this, " failed because the singleton value was set to ", singleton, " instead!");
        throw EventError("Attempted to deregister singleton while another one was assigned.");
    }
    setGlobalInstance(nullptr);
}

void EventSystem::deliverEvents()
{
    Log::writeToLog(Log::INFO, "EventSystem delivery thread startup successful.");
    while (true)
    {
        if (shutdownFlag == true)
        {
            return;
        }

        bool empty = false;
        while (empty == false)
        {
            std::unique_ptr<Event> top_event;
            {
                std::lock_guard<std::mutex> lock(queueMux);

                if (events.empty())
                {
                    empty = true;
                    continue;
                }

                top_event.swap(events.front());
                events.pop_front();
            }

            // Now deliver the event to callbacks in the queue
            {
                std::lock_guard<std::mutex> lock(callbackMux);

                for (EventReceiver* callback : callbacks)
                {
                    HandleResult result = HandleResult::Unhandled;
                    for (auto dispatcher : callback->dispatchers)
                    {
                        result = dispatcher(callback, top_event.get());
                        if (result == HandleResult::Stop || result == HandleResult::Error)
                        {
                            break;
                        }
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}




void EventSystem::registerCallback(EventReceiver* callback)
{
    std::lock_guard<std::mutex> lock(callbackMux);
    if (callbacks.insert(callback).second == false)
    {
        Log::writeToLog(Log::WARN, "Event callback class ", callback, " already registered! Ignoring.");
    } else {
        Log::writeToLog(Log::L_DEBUG, "Registered event callback class ", callback);
    }
}

void EventSystem::deregisterCallback(EventReceiver* callback)
{
    std::lock_guard<std::mutex> lock(callbackMux);
    auto it = callbacks.find(callback);

    if (it == callbacks.end())
    {
        Log::writeToLog(Log::ERR, "Attempted to remove event callback ", callback, " that was not registered!");
        throw EventError("Removal of unregistered event callback attempted!");
    }

    callbacks.erase(it);

    Log::writeToLog(Log::L_DEBUG, "Deregistered event system callback class ", callback);
}

void EventSystem::internalQueueEvent(std::unique_ptr<Event>&& event)
{
    // Check if this is an envelope event. If so, pass it through the network
    if (event->i_category == Events::Network && event->i_id == Events::Net::Envelope)
    {
        EnvelopeMessage* message = (EnvelopeMessage*)event.get();

        if (!network)
        {
            Log::writeToLog(Log::ERR, "Attempted to deliver an envelope when no network setup!");
            throw EventError("Attempted to deliver an envelope without an active network!");
        }
        RakNet::RakNetGUID destination = message->address;
        if (destination == RakNet::UNASSIGNED_RAKNET_GUID)
        {
            destination = network->getFirstConnectionGUID();
        }
        network->sendMessage(destination, message, PacketReliability::RELIABLE_SEQUENCED);
    }

    std::lock_guard<std::mutex> lock(queueMux);
    events.push_back(std::move(event));
}


    
