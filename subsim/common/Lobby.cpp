#include "Lobby.h"

#include "BitStreamHelper.h"

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
