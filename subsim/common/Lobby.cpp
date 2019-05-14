#include "Lobby.h"

#include "BitStreamHelper.h"

#include "Messages.h"

LobbyStatusRequest::LobbyStatusRequest() {}

LobbyStatusRequest::LobbyStatusRequest(RakNet::BitStream& stream)
{
    deserialize(stream);
}

RakNet::MessageID LobbyStatusRequest::getType() const
{
    return MessageType::ID_LOBBY_STATUS_REQUEST;
}

void LobbyStatusRequest::deserialize(RakNet::BitStream& stream)
{
    uint32_t size;
    stream >> size;
    stations.reserve(size);
    for (int i = 0; i < size; ++i)
    {
        StationID id;
        bool update;
        stream >> id.team >> id.unit >> id.station >> update;

        stations.push_back(std::pair<StationID, bool>(id, update));
    }
}

void LobbyStatusRequest::serialize(RakNet::BitStream& stream) const
{
    uint32_t size = stations.size();
    stream << size;
    for (auto pair : stations)
    {
        stream << pair.first.team << pair.first.unit
            << pair.first.station << pair.second;
    }
}

RakNet::MessageID LobbyStatus::getType() const
{
    return MessageType::ID_LOBBY_STATUS;
}

LobbyStatus::LobbyStatus() {}

LobbyStatus::LobbyStatus(RakNet::BitStream& stream)
{
    deserialize(stream);
}

void LobbyStatus::deserialize(RakNet::BitStream& stream)
{
    stream >> stations;
}

void LobbyStatus::serialize(RakNet::BitStream& stream) const
{
    stream << stations;
}
