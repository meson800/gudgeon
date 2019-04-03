#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <set>
#include <memory>
#include <mutex>
#include <thread>

/*!
 * Class from which Events all inherit from. Note that because
 * events are stored polymorphically by the EventSystem, your
 * custom Event should have a virtual deconstructor.
 */
class Event
{
    public:
    /*!
     * Default Event constructor that sets public accessible variables
     * that are used for dispatch.
     */
    Event(uint32_t category_, uint32_t id_);
    virtual ~Event();

    /// Stores the broad category of the event
    constexpr static uint32_t category = 0;

    /**
     * Stores the specific event ID. This is used to determine which event type
     * to cast this to.
     */
    constexpr static uint32_t id = 0;

    // These variables should be automatically set by the Event constructor.
    /// Stores instance-level event category
    uint32_t i_category;
    /// Stores instance-level event ID
    uint32_t i_id;
};

/*!
 * Enum describing the various return values an event handler
 * should return
 */
enum HandleResult
{
    Stop,     // The handler processed the event and the event should stop propogation
    Continue, // This handler processed the event, but the event should continue propogation
    Error,    // The handler could not handle the event due to an exceptional situation, event should stop propogation
    Unhandled // The handler could not handle this event, propogation should continue
};

/*!
 * Interface for receiving events. Inherit from this
 * if you want to receive events.
 */
class EventReceiver
{
public:
    /// Hooks this class into the event system
    EventReceiver(HandleResult(*_dispatcher)(EventReceiver* handler, Event* event));

    /// Removes this class from the event system
    virtual ~EventReceiver();

    /// Handles a given event. You can use the dispatchEvent template to implement this for your class.
    HandleResult(*dispatcher)(EventReceiver* handler, Event* event);
};

/*!
 * This is a helper template that handles dispatch/casting into member types.
 * Once you define an EventReciever, initalize it with a function pointer to
 * this type, once you include the various destination functions as template parameters
 *
 * The expected type handler functions are member functions that take a pointer
 * to the event type, and return a HandleResult.
 */
 template <typename Handler, typename T, HandleResult(Handler::*hfunc)(T*), typename ...args>
 HandleResult dispatchEvent(EventReceiver* handler, Event* event)
 {
    if (event->i_category == T::category && event->i_id == T::id)
    {
        return (static_cast<Handler*>(handler)->*hfunc)((T*)event);
    } else {
        return dispatchEvent<Handler, args...>(handler, event);
    }
 }
 
 /*!
  * Base case of dispatchEvent
  */
template <typename Handler>
HandleResult dispatchEvent(EventReceiver* handler, Event* event)
{
    //base case, abort!
    return HandleResult::Unhandled;
}





/*!
 * This class handles all of the "plumbing" between various modules.
 *
 * It stores callbacks to various classes wanting to recieve callbacks,
 * and gives a simple interface for delivering messages to others.
 *
 * Other classes can register to recieve certain "categories" of events.
 * Classes also register with a priority, wither higher priority
 * classes recieving events first.
 */
class EventSystem
{
public:
    /// Returns the global event handler pointer, useful for delivering events
    static EventSystem* getGlobalInstance();

    /// Sets up internal EventSystem state, making it ready to deliver events
    EventSystem();

    /// Deregisters on deconstruction
    ~EventSystem();

    /// Delete copy constructor 
    EventSystem(const EventSystem& other) = delete;

    /// Delete copy assignment
    EventSystem& operator=(const EventSystem& other) = delete;

    /// Adds a given event receiver callback class to the delivery queue
    void registerCallback(EventReceiver* receiver);

    /// Removes a given event receiver callback calss from the delivery queue
    void deregisterCallback(EventReceiver* receiver);

    /**
     * Templated deliver event call that makes a copy of the event before
     * queueing it for delivery. This is used because, for polymorphism to work,
     * pointers are thrown around. Using this templated delivery class ensures
     * that this is done safely (because delivery may happen afte the original event
     * has been deconstructed)
     */
    template<typename T>
    void queueEvent(T event)
    {
        // Make a copy on the heap so we can manage it
        T* copy = new T(event);

        std::unique_ptr<Event> copy_ptr(copy);

        internalQueueEvent(std::move(copy_ptr));
    }

private:
    /// Sets the global event handler pointer
    static void setGlobalInstance(EventSystem* system);

    static EventSystem* singleton;

    /// Takes a given unique pointer (a moveable value) and stores it into the queue
    void internalQueueEvent(std::unique_ptr<Event>&& event);

    /// Loops until shutdwonFlag is true, delivering events on its own thread
    void deliverEvents();

    /// Set that stores all active callbacks
    std::set<EventReceiver*> callbacks;

    /// Deque that stores all events to be delivered
    std::deque<std::unique_ptr<Event>> events;

    /// Mutex that protects the queue
    std::mutex queueMux;

    /// Mutex that protects the callback class
    std::mutex callbackMux;

    /// Atomic that indicates when shutdown should occur
    std::atomic<bool> shutdownFlag;

    /// Thread that delivers events to event receivers
    std::thread deliveryThread;
};
