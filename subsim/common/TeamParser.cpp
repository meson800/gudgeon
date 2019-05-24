#include "TeamParser.h"

#include "Exceptions.h"
#include "Log.h"

#include <sstream>

 std::map<uint16_t,Team_t> TeamParser::parseStations(const ParseResult& parse)
 {
    using Station_t = std::pair<uint16_t, StationType>;

    std::map<uint16_t, Team_t> result;

    // Read in all of the teams
    auto range = parse.equal_range("TEAM");

    for (auto it = range.first; it != range.second; ++it)
    {
        std::string teamName;
        uint16_t id = 0;
        for (auto keyValIt = it->second.cbegin(); keyValIt != it->second.cend(); ++keyValIt)
        {
            const std::string& key = keyValIt->first;
            const std::vector<std::string>& values = keyValIt->second;

            if (values.size() != 1)
            {
                Log::writeToLog(Log::ERR, "Unexpected amount of values in key:\"", key, "\". Number of values:", values.size());
                throw TeamParseError("Invalid number of values encountered when parsing a TEAM section.");
            }

            if (key == "name")
            {
                teamName = values.at(0);
            } else if (key == "id") {
                std::istringstream sstream(values.at(0));
                sstream >> id;
            } else if (key == "flag_score" || key == "death_score") {
                //ignore, other parser will handle it
            } else {
                Log::writeToLog(Log::ERR, "Unexpected key in TEAM section: ", key);
                throw TeamParseError("Invalid key encountered when parsing a TEAM section.");
            }
        }

        // Check for invalid ID
        if (id == 0)
        {
            Log::writeToLog(Log::ERR, "Team ID not provided or ID set equal to zero for team with name:", teamName);
            throw TeamParseError("Invalid ID set/left unset when parsing a TEAM section.");
        }
        result[id] = Team_t(teamName, std::vector<Unit_t>{});
        Log::writeToLog(Log::L_DEBUG, "Successfully processed team with ID=", id, " and name=\"", teamName, "\"");
    }

    // Now read in units
    range = parse.equal_range("UNIT");

    for (auto it = range.first; it != range.second; ++it)
    {
        std::string unitName;
        uint16_t team;
        std::vector<StationType> stations;

        for (auto keyValIt = it->second.cbegin(); keyValIt != it->second.cend(); ++keyValIt)
        {
            const std::string& key = keyValIt->first;
            const std::vector<std::string>& values = keyValIt->second;

            if (key == "name")
            {
                if (values.size() > 1)
                {
                    Log::writeToLog(Log::WARN, "Extra name key/vals encountered for unit while processing team file!");
                }

                unitName = values.at(0);
            } else if (key == "team") {
                if (values.size() != 1)
                {
                    Log::writeToLog(Log::ERR, "Unexpected number of team key/vals given for a unit: ", values.size(), " (expected 1)");
                    throw TeamParseError("Unexpected number of team keyvals given for a unit");
                }
                std::istringstream sstream(values.at(0));
                sstream >> team;
            } else if (key == "station") {
                for (const std::string& name : values)
                {
                    if (StationTypeLookup.count(name) != 1)
                    {
                        Log::writeToLog(Log::ERR, "Station type:", name, " is unknown.");
                        throw TeamParseError("Invalid station type referenced.");
                    }

                    // otherwise, add to station list
                    stations.push_back(StationTypeLookup.at(name));
                }
            } else {
                Log::writeToLog(Log::ERR, "Unexpected key in UNIT section: ", key);
                throw TeamParseError("Invalid key encountered while parsing a UNIT sectoin.");
            }
        }

        // check for invalid state
        if (result.count(team) != 1)
        {
            Log::writeToLog(Log::ERR, "Unit with name=\"", unitName, "\" had invalid team ID=", team);
            throw TeamParseError("Unit had invalid team ID.");
        }
        std::ostringstream stationConcat;
        for (auto station : stations)
        {
            stationConcat << StationNames[station] << ",";
        }
            
            
        Log::writeToLog(Log::L_DEBUG, "Succesfully processed unit with name=\"", unitName, "\" for team ", team,
         ". Included stations:", stationConcat.str().substr(0, stationConcat.str().length() - 1));
        result[team].second.push_back(Unit_t(unitName, stations));
    }
    return result;
 }

std::map<uint16_t, std::pair<uint16_t, uint16_t>> TeamParser::parseScoring(const ParseResult& parse)
{
    std::map<uint16_t, std::pair<uint16_t, uint16_t>> result;
    auto range = parse.equal_range("TEAM");

    for (auto it = range.first; it != range.second; ++it)
    {

        uint16_t id = 0;
        uint16_t flagScore = 0;
        uint16_t deathScore = 0;
        bool set = false;
        for (auto keyValIt = it->second.cbegin(); keyValIt != it->second.cend(); ++keyValIt)
        {
            const std::string& key = keyValIt->first;
            const std::vector<std::string>& values = keyValIt->second;

            if (values.size() != 1)
            {
                Log::writeToLog(Log::ERR, "Unexpected number of values in key:\"", key, "\". Number of values:", values.size());
                throw TeamParseError("Invalid number of keys when reading team score information");
            }

            std::istringstream sstream(values.at(0));
            if (key == "id") {
                sstream >> id;
            } else if (key == "flag_score") {
                set = true;
                sstream >> flagScore;
            } else if (key == "death_score") {
                set = true;
                sstream >> deathScore;
            }
        }
        if (set)
        {
            result[id] = std::pair<uint16_t, uint16_t>(flagScore, deathScore);
        }
    }
    return result;
 }
