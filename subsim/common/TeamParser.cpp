#include "TeamParser.h"

#include "Exceptions.h"
#include "Log.h"

 std::vector<std::pair<uint16_t, StationType>> TeamParser::parseStations(const ParseResult& parse)
 {
    using Station_t = std::pair<uint16_t, StationType>;

    std::vector<Station_t> result;

    auto range = parse.equal_range("TEAM");

    uint16_t teamID = 0;
    for (auto it = range.first; it != range.second; ++it)
    {
        for (auto keyValIt = it->second.cbegin(); keyValIt != it->second.cend(); ++keyValIt)
        {
            const std::string& key = keyValIt->first;
            const std::vector<std::string>& values = keyValIt->second;
            if (key != "station")
            {
                Log::writeToLog(Log::ERR, "Unexpected key in TEAM section: ", key);
                throw TeamParseError("Invalid key encountered while parsing a team file");
            }

            if (values.size() != 1)
            {
                Log::writeToLog(Log::ERR, "Unexpected amount of values in \"station\" key. Number of values:", values.size());
                throw TeamParseError("Invalid number of valids encountered while parsing a team file.");
            }

            if (StationTypeLookup.count(values.at(0)) != 1)
            {
                Log::writeToLog(Log::ERR, "Station type:", values.at(0), " is unknown.");
                throw TeamParseError("Invalid station type referenced.");
            }

            result.push_back(Station_t(teamID, StationTypeLookup.at(values.at(0))));
        }
        ++teamID;
    }
    return result;
 }
