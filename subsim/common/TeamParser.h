#pragma once

#include <vector>
#include <map>

#include "GenericParser.h"

#include "Stations.h"

/// Typedef for a unit type
using Unit_t = std::pair<std::string, std::vector<StationType>>;
using Team_t = std::pair<std::string, std::vector<Unit_t>>;
/**
 * This class takes a ParseResult from the generic parser and
 * turns it into an assignment of station IDs. Stations
 * are distributed per unit, so we return a top level map
 * that contains teams, which are split into vectors of units.
 *
 * A station ID is an std::pair<uint16_t, StationType>
 */
class TeamParser
{
public:
    static std::map<uint16_t, Team_t> parseStations(const ParseResult& parse);
};
