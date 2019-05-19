#pragma once

#include "MessageIdentifiers.h"
#include "RakNetTypes.h"

#include "EventSystem.h"
#include "EventID.h"

#include <cstdint>
#include <memory>

namespace RakNet
{
    /// Forward declaration of BitStream
    class BitStream;
}

enum MessageType
{
    ID_VERSION = ID_USER_PACKET_ENUM + 1,
    ID_VERSION_MISMATCH,
    ID_LOBBY_STATUS_REQUEST,
    ID_LOBBY_STATUS,
    ID_ENVELOPE,
};

/*!
 * MessageInterface stores serialization, deserialization, and type classes.
 * This makes it straightforward to send classes inherited from MessageInterface
 */
class MessageInterface
{
public:
    /// Function for getting the MessageType of this message
    virtual RakNet::MessageID getType() const = 0;
    
    /// Function that sets class data from a BitStream
    virtual void deserialize(RakNet::BitStream& stream) = 0;

    /// Stores class data into a BitStream
    virtual void serialize(RakNet::BitStream& stream) const = 0;
};


/*!
 * Class that supports serializing/deserializing to BitStreams
 * to a VerisonMessage type
 */
struct VersionMessage
{
    uint32_t versionMajor;
    uint32_t versionMinor;

    VersionMessage(uint32_t major, uint32_t minor);
    VersionMessage(RakNet::BitStream& source);

    void deserialize(RakNet::BitStream& source);
    void serialize(RakNet::BitStream& source);
};

/*!
 * Class that supports serializing/deserializing an Event, by enclosing it in an envelope
 */
struct EnvelopeMessage : public MessageInterface, public Event
{
    RakNet::RakNetGUID address;

    std::shared_ptr<Event> event;

    EnvelopeMessage(RakNet::BitStream& source, RakNet::RakNetGUID address_ = RakNet::UNASSIGNED_RAKNET_GUID);

    EnvelopeMessage()
        : Event(category, type)
    {}

    template <typename T>
    EnvelopeMessage(const T& event_, RakNet::RakNetGUID address_ = RakNet::UNASSIGNED_RAKNET_GUID)
        : address(address_)
        , event(new T(event_))
        , Event(category, type)
    {}

    RakNet::MessageID getType() const override;
    void deserialize(RakNet::BitStream& source) override;
    void serialize(RakNet::BitStream& source) const override;

    constexpr static uint32_t category = Events::Category::Network;
    constexpr static uint32_t type = Events::Net::Envelope;
};
    
