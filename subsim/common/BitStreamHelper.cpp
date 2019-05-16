#include "BitStreamHelper.h"

RakNet::BitStream& operator<<(RakNet::BitStream& stream, const std::string& val)
{
    uint32_t len = val.length();

    stream.Write(len);
    stream.Write((char*)val.c_str(), len);
    return stream;
}

RakNet::BitStream& operator>>(RakNet::BitStream& stream, std::string& val)
{
    uint32_t len;
    if (!stream.Read(len))
    {
        Log::writeToLog(Log::ERR, "Unable to deserialize string length!");
        throw NetworkMessageError("Deserialization failure!");
    }
    char* buf = new char[len];

    if (!stream.Read(buf, len))
    {
        Log::writeToLog(Log::ERR, "Unable to deserialize string contents!");
        throw NetworkMessageError("Deserialization failure!");
    }

    val = std::string(buf, len);
    delete[] buf;

    return stream;
}
