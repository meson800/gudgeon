#include "LobbyHandler.h"

#include "../common/Log.h"

#include "../common/TeamParser.h"

LobbyHandler::LobbyHandler()
{
    std::vector<std::pair<uint16_t, StationType>> requestedStations;
    TeamParser::parseStations(GenericParser::parse("data/test_game.cfg"));

    for (auto station : requestedStations)
    {
        status.stations[station] = RakNet::UNASSIGNED_RAKNET_GUID;
    }
}

bool LobbyHandler::ConnectionLost(RakNet::RakNetGUID other)
{
    return false;
}

bool LobbyHandler::LobbyStatusRequested(RakNet::RakNetGUID other, const LobbyStatusRequest& request)
{
    /* Check to see if this client is in the lobby list yet */
    if (waitingSystems.count(other) == 0)
    {
        waitingSystems.insert(other);
        // Also insert into the "client to stations" map, so we know if all stations used
        status.clientToStations[other] = request.stations.size();

    }

    /* Make sure that we didn't change the number of stations */
    if (status.clientToStations[other] != request.stations.size())
    {
        Log::writeToLog(Log::WARN, "Lobby status request from system:", other, " invalid!",
            " Number of stations changed from ", status.clientToStations[other], " to ", request.stations.size(), "!");
        return true;
    }

    /* Update the LobbyStatus if possible
     * Do this using the idea of a "transaction". Only if all modifications/updates
     * go through should we touch the actual status
     */
    auto rollback = status.stations;

    bool transaction_fail = false;
    for (auto station : request.stations)
    {
        // Check to see if this is null/unassigned station. Ignore it if so
        if (station.second == StationType::Unassigned)
        {
            continue;
        }

        /* Check for error conditions. This isn't assignable if either
         * the requested station doesn't exist or is already assigned to
         * someone else
         */
        if (status.stations.count(station) == 0 ||
            (status.stations[station] != other && 
             status.stations[station] != RakNet::UNASSIGNED_RAKNET_GUID))
        {
            transaction_fail = true;
            Log::writeToLog(Log::WARN, "Lobby status request transaction failed, for"
                " requested station on team:", station.first, " and station:", StationNames[station.second],
                " from client ", other);
            break;
        }

        // Otherwise, assign the station as normal
        status.stations[station] = other;
    }

    if (transaction_fail)
    {
        // Rollback so we don't leave the lobby in an invalid state
        status.stations = rollback;
    }

    // Send the updated lobby status to all connected clients.
    for (auto system : waitingSystems)
    {
        network->sendMessage(system, &status, PacketReliability::RELIABLE_SEQUENCED);
    }
}

bool LobbyHandler::UpdatedLobbyStatus(const LobbyStatus& status)
{
    return false;
}
