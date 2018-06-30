#include "Messages.h"

#include "BitStream.h"

#include "Exceptions.h"
#include "Log.h"

VersionMessage::VersionMessage(uint32_t major, uint32_t minor)
    : versionMajor(major)
    , versionMinor(minor)
{}

VersionMessage::VersionMessage(RakNet::BitStream& source)
{
    deserialize(source);
}

void VersionMessage::deserialize(RakNet::BitStream& source)
{
    if (!source.Read(versionMajor) || !source.Read(versionMinor))
    {
        Log::writeToLog(Log::ERR, "Unable to deserialize a VersionMessage");
        throw NetworkMessageError("VersionMessage deserialization failure!");
    }
}

void VersionMessage::serialize(RakNet::BitStream& source)
{
    source.Write(versionMajor);
    source.Write(versionMinor);
}
