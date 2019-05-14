#pragma once

#include <vector>
#include <map>

#include "BitStream.h"
#include "Exceptions.h"
#include "Log.h"

template<typename T>
RakNet::BitStream& operator<<(RakNet::BitStream& stream, T val)
{
    stream.Write(val);
    return stream;
}

template<typename T>
RakNet::BitStream& operator>>(RakNet::BitStream& stream, T& val)
{
    if (!stream.Read(val))
    {
        Log::writeToLog(Log::ERR, "Unable to deserialize!");
        throw NetworkMessageError("Deserialization failure!");
    }
    return stream;
}


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
    

template<typename T>
RakNet::BitStream& operator<<(RakNet::BitStream& stream, const std::vector<T>& vec)
{
    uint32_t size = vec.size();
    stream << size;

    for (auto val : vec)
    {
        stream << val;
    }
    return stream;
}

template<typename T>
RakNet::BitStream& operator>>(RakNet::BitStream& stream, std::vector<T>& vec)
{
    uint32_t size;
    stream >> size;

    vec.reserve(size);
    for (uint32_t i = 0; i < size; ++i)
    {
        T val;
        stream >> val;
        vec.push_back(val);
    }
    
    return stream;
}

template<typename T, typename U>
RakNet::BitStream& operator<<(RakNet::BitStream& stream, const std::pair<T,U>& val)
{
    stream << val.first << val.second;
    return stream;
}

template<typename T, typename U>
RakNet::BitStream& operator>>(RakNet::BitStream& stream, std::pair<T,U>& val)
{
    stream >> val.first >> val.second;
    return stream;
}

template<typename T, typename U>
RakNet::BitStream& operator<<(RakNet::BitStream& stream, const std::map<T, U>& val)
{
    uint32_t size = val.size();
    stream << size;

    for (auto pair : val)
    {
        stream << pair.first << pair.second;
    }
    return stream;
}

template<typename T, typename U>
RakNet::BitStream& operator>>(RakNet::BitStream& stream, std::map<T, U>& val)
{
    uint32_t size;
    stream >> size;

    for (uint32_t i = 0; i < size; ++i)
    {
        T key;
        U value;

        stream >> key >> value;
        val[key] = value;
    }

    return stream;
}
        
