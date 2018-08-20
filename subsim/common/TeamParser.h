#pragma once

#include <vector>

#include "GenericParser.h"

#include "Stations.h"

/**
 * This class takes a ParseResult from the generic parser and
 * turns it into a vector of station IDs, as required by
 * the Lobby.
 *
 * A station ID is an std::pair<uint16_t, StationType>
 */
class TeamParser
{
public:
    static std::vector<std::pair<uint16_t, StationType>> parseStations(const ParseResult& parse);
};
