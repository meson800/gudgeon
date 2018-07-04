#pragma once

#include <map>
#include <vector>

#include "RakNetTypes.h"

#include "Stations.h"

/*!
 * Stores the current lobby status. This class can be
 * serialized and deserialized over the network in order to
 * inform clients of changes in status.
 *
 * This stores all stations required, along with the status
 * of each station. We use RakNetGUID to identify stations.
 */
class LobbyStatus
{
public:
    /*! 
     * Stores the current occupancy of each possible station.
     * The GUID term is RakNet::UNASSIGNED_RAKNET_GUID for free stations
     */
    std::map<uint16_t, std::pair<StationType, RakNet::RakNetGUID>> stations;

    /// Inits the LobbyStatus state from a BitStream
    LobbyStatus(RakNet::BitStream& stream);
    
    /// Clears the station map and sets it using the BitStream data
    void deserialize(RakNet::BitStream& stream);

    /// Records this LobbyStatus into a BitStream
    void serialize(RakNet::BitStream& stream) const;
};
