#pragma once

#include "MessageIdentifiers.h"
#include "RakNetTypes.h"

#include <cstdint>

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
    ID_LOBBY_STATUS
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
    
