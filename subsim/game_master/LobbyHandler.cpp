#include "LobbyHandler.h"

#include "../common/Log.h"

#include "../common/TeamParser.h"

#include "../common/SimulationEvents.h"

LobbyHandler::LobbyHandler(const ParseResult& parse)
{
    std::vector<std::pair<uint16_t, StationType>> requestedStations;
    std::map<uint16_t, Team_t> parsedStations = TeamParser::parseStations(parse);

using Unit_owner_t = std::pair<std::string, std::vector<std::pair<StationType, RakNet::RakNetGUID>>>;
using Team_owner_t = std::pair<std::string, std::vector<Unit_owner_t>>;
    
    // Build up the lobby status
    for (auto& team_pair : parsedStations)
    {
        status.stations[team_pair.first] = Team_owner_t(team_pair.second.first, std::vector<Unit_owner_t>());

        for (auto& unit_pair : team_pair.second.second)
        {
            std::vector<std::pair<StationType, RakNet::RakNetGUID>> initialState;
            for (StationType type : unit_pair.second)
            {
                initialState.push_back(std::pair<StationType, RakNet::RakNetGUID>(type, RakNet::UNASSIGNED_RAKNET_GUID));
            }
            status.stations[team_pair.first].second.push_back(Unit_owner_t(unit_pair.first, initialState));
        }
    }
}

bool LobbyHandler::ConnectionLost(RakNet::RakNetGUID other)
{
    // Unassign all stations from this GUID.
    Log::writeToLog(Log::INFO, "Client ", other, " disconnected from the lobby.");
    for (auto& team_pair : status.stations)
    {
        for (auto& unit_pair : team_pair.second.second)
        {
            for (auto& station_pair : unit_pair.second)
            {
                if (station_pair.second == other)
                {
                    Log::writeToLog(Log::INFO, "Unassigning station ", StationNames[station_pair.first], " on unit ", 
                        unit_pair.first, " on team ID", team_pair.first, " because client ", other, " disconnected.");
                    station_pair.second = RakNet::UNASSIGNED_RAKNET_GUID;
                }
            }
        }
    }

    // Remove this system from the game master.
    if (waitingSystems.count(other) == 1)
    {
        waitingSystems.erase(other);
    }

    // Update all stations
    for (auto system : waitingSystems)
    {
        network->sendMessage(system, &status, PacketReliability::RELIABLE_SEQUENCED);
    }

    return true;
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

    // Check if the lobby status has things to validly update
    auto rollback = status.stations;

    bool transaction_fail = false;
    for (auto station_assign_pair : request.stations)
    {
        uint16_t team = station_assign_pair.first.team;
        uint16_t unit = station_assign_pair.first.unit;
        uint16_t station = station_assign_pair.first.station;

        // Check if the station that they want is actually in range
        if (status.stations.count(team) != 1)
        {
            transaction_fail = true;
            Log::writeToLog(Log::WARN, "Lobby status request transaction failed"
                " requested station on team:", team , " which does not exist, from client ", other);
            break;
        }

        if (unit > status.stations[team].second.size())
        {
            transaction_fail = true;
            Log::writeToLog(Log::WARN, "Lobby status request transaction failed, client ", other,
                " requested unit ", unit, " on team ", team, " which does not exist!");
            break;
        }

        if (station > status.stations[team].second[unit].second.size())
        {
            transaction_fail = true;
            Log::writeToLog(Log::WARN, "Lobby status request transaction failed, client ", other,
                " requested station ", station, " on team ", team, " and unit ", unit, " which does not exist!");
            break;
        }

        // Make sure the station is unowned if trying to own
        RakNet::RakNetGUID currentOwner = status.stations[team].second[unit].second[station].second;
        if (station_assign_pair.second == true && currentOwner != RakNet::UNASSIGNED_RAKNET_GUID)
        {
            transaction_fail = true;
            Log::writeToLog(Log::WARN, "Lobby status request transaction failed, client ", other,
                " requested station ", station, " on unit ", unit, " in team ", team, " but it was already owned by client ", currentOwner);
            break;
        }

        if (station_assign_pair.second == false && currentOwner != other)
        {
            transaction_fail = true;
            Log::writeToLog(Log::WARN, "Lobby status request transaction failed, client ", other,
                " tried to release station ", station, " on unit ", unit, " in team ", team, " but it was not owned by them! Currently owned by client ", currentOwner);
            break;
        }

        // Finally do the transaction
        if (station_assign_pair.second == true)
        {
            status.stations[team].second[unit].second[station].second = other;
        } else {
            status.stations[team].second[unit].second[station].second = RakNet::UNASSIGNED_RAKNET_GUID;
        }
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

    // Check if all stations assigned.
    // Accumulate the vector of station assignments per RakNet ID.
    std::map<RakNet::RakNetGUID, std::vector<SimulationStart::Station>> assignments;
    std::map<uint32_t, std::vector<std::vector<std::pair<StationType, RakNet::RakNetGUID>>>> serverAssignments;

    bool done = true;
    for (auto& team_pair : status.stations)
    {
        uint32_t unit = 0;
        for (auto& unit_pair : team_pair.second.second)
        {
            serverAssignments[team_pair.first].push_back(std::vector<std::pair<StationType, RakNet::RakNetGUID>>());
            for (auto& station_pair : unit_pair.second)
            {
                if (station_pair.second == RakNet::UNASSIGNED_RAKNET_GUID)
                {
                    // We aren't done. Just return
                    done = false;
                    continue;
                }
                //otherwise, accumulate this assignment
                SimulationStart::Station station;
                station.team = team_pair.first;
                station.unit = unit;
                station.station = station_pair.first;
                assignments[station_pair.second].push_back(station);

                serverAssignments[team_pair.first][unit].push_back(station_pair);
            }
            ++unit;
        }
    }

    if (done)
    {
        Log::writeToLog(Log::INFO, "Lobby creation completed; all stations assigned. Sending SimulationStart messages.");
        for (auto& pair : assignments)
        {
            Log::writeToLog(Log::L_DEBUG, "Sending SimulationStart event to client ", pair.first, " who owns ", pair.second.size(), " stations.");
            // Create the SimStart event
            SimulationStart sstart;
            sstart.stations = pair.second;

            EnvelopeMessage envelope(sstart, pair.first);
            // deliver simstart's to all attached clients!
            EventSystem::getGlobalInstance()->queueEvent(envelope);
        }

        // Now, send a SimStart command to ourselves
        SimulationStartServer serverStart;
        serverStart.assignments = serverAssignments;
        EventSystem::getGlobalInstance()->queueEvent(serverStart);
    }
            

}

bool LobbyHandler::UpdatedLobbyStatus(const LobbyStatus& status)
{
    return false;
}
