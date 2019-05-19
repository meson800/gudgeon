#pragma once

#include <map>
#include <vector>

#include "RakNetTypes.h"

#include "Stations.h"

#include "Messages.h"

using Unit_owner_t = std::pair<std::string, std::vector<std::pair<StationType, RakNet::RakNetGUID>>>;
using Team_owner_t = std::pair<std::string, std::vector<Unit_owner_t>>;

/*!
 * Stores the number of stations this client can support,
 * along with their assignments
 *
 * This gets sent to show our intrest in the lobby, and
 * to update our lobby information
 */
class LobbyStatusRequest : public MessageInterface
{
public:
    /*!
     * Struct that stores a station identifier (team ID/unit ID/station ID)
     */
    struct StationID
    {
        uint16_t team;
        uint16_t unit;
        uint16_t station;
    };

    /*! 
     * Stores the assignment of each station. This transmits our "update" command.
     * Setting the bool = true means assign this station to us, setting to false means
     * deassign it from us.
     */
    std::vector<std::pair<StationID, bool>> stations;

    LobbyStatusRequest();
    LobbyStatusRequest(RakNet::BitStream& stream);
    virtual RakNet::MessageID getType() const override;
    virtual void deserialize(RakNet::BitStream& stream) override;
    virtual void serialize(RakNet::BitStream& stream) const override;
};

/*!
 * Stores the current lobby status. This class can be
 * serialized and deserialized over the network in order to
 * inform clients of changes in status.
 *
 * This stores all stations required, along with the status
 * of each station. We use RakNetGUID to identify stations.
 */
class LobbyStatus : public MessageInterface
{
public:
    /*! 
     * Stores the current occupancy of each possible station.
     * The GUID term is RakNet::UNASSIGNED_RAKNET_GUID for free stations
     */
    std::map<uint16_t, Team_owner_t> stations;

    /*!
     * Stores the number of stations each connected client supports
     */
    std::map<RakNet::RakNetGUID, uint8_t> clientToStations;

    /// Returns message type
    virtual RakNet::MessageID getType() const override;

    /// Inits an empty LobbyStatus
    LobbyStatus();

    /// Inits the LobbyStatus state from a BitStream
    LobbyStatus(RakNet::BitStream& stream);
    
    /// Clears the station map and sets it using the BitStream data
    virtual void deserialize(RakNet::BitStream& stream) override;

    /// Records this LobbyStatus into a BitStream
    virtual void serialize(RakNet::BitStream& stream) const override;
};
