#pragma once

#include "MessageIdentifiers.h"

#include <cstdint>

namespace RakNet
{
    /// Forward declaration of BitStream
    class BitStream;
}

enum MessageType
{
    ID_VERSION = ID_USER_PACKET_ENUM + 1,
    ID_VERSION_MISMATCH
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
    
