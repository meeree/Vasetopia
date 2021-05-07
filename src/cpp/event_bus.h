#pragma once 

#include <memory>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <iostream>

/// Event base class.
struct Event { virtual ~Event () {} };

/// Abstract class that handles events.
struct EventHandler 
{
    virtual void Handle(std::shared_ptr<Event> e) = 0;
    virtual ~EventHandler () {}
};

/// An event bus relays information from publishers to subscribers when events happen according to the the pub-sub pattern.
/// It is a singleton class.
class EventBus 
{
private:
    static std::unique_ptr<EventBus> ms_singleton;
    std::unordered_map<size_t, std::vector<std::shared_ptr<EventHandler>>> subscriptions;

    /// Note that the constructor is private because this a singleton.
    EventBus () = default;
public:
    /// Create the singleton instance. NOTE THIS IS NECESSARY BEFORE USING EVENT US!
    static void CreateSingleton () {ms_singleton.reset(new EventBus);}

    /// Register a subscription for an event handler that expects to get events of a certain ID.
    /// \param [in] eid Event ID. This can be retrieved from Event e by running EventBus::GetID<Event>().
    /// \param [in] handler Event handler pointer.
    static void Subscribe (size_t eid, std::shared_ptr<EventHandler> handler) 
    {
        ms_singleton->subscriptions[eid].push_back(handler);
    }

    /// Publish an event to all subscribers.
    /// \param [in] event The event itself.
    /// \param [in] eid Event ID. This can be retrieved from Event by running EventBus::GetID<Event>().
    static void Publish (std::shared_ptr<Event> e, size_t eid)
    {
        for(auto& handler: ms_singleton->subscriptions[eid])
            handler->Handle(e);
    }

    /// Get ID associated with an event.
    template <typename EventT>
    static size_t GetID ()
    {
        return std::type_index(typeid(EventT)).hash_code();
    }
};
std::unique_ptr<EventBus> EventBus::ms_singleton;

class TestEvent : public Event
{
public: 
    std::string name = "bob";
};

class TestWrapper 
{
    void SetTitle (std::string new_title)
    {
        title = new_title;
        std::cout << "Set TestWrapper title to : " << new_title << std::endl;
    }

    class TestHandler : public EventHandler
    {
    public:
        TestWrapper* wrapper;
        virtual void Handle (std::shared_ptr<Event> e) override
        {
            auto e_test = std::static_pointer_cast<TestEvent>(e);
            std::cout << "Event name : " << e_test->name << std::endl;
            wrapper->SetTitle("New Title");
        }
    };
    std::shared_ptr<TestHandler> handler;
    std::string title = "";

public:
    TestWrapper () 
        : handler{new TestHandler}
    {
        handler->wrapper = this;
        EventBus::Subscribe(EventBus::GetID<TestEvent>(), handler);

        auto eptr = std::shared_ptr<Event>(new Event);
        EventBus::Publish(eptr, EventBus::GetID<TestEvent>());
    }
};
