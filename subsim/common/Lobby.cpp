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
    stream >> stations;
}

void LobbyStatusRequest::serialize(RakNet::BitStream& stream) const
{
    stream << stations;
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
