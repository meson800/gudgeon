#pragma once

#include <vector>
#include <map>

#include "GenericParser.h"

class Terrain
{
public:
    std::vector<uint8_t> map;
    uint32_t width;
    uint32_t height;
    uint32_t scale;
};

class Config
{
public:
    Terrain terrain;
};

/**
 * This class takes a ParseResult from the generic parser and
 * turns it into the information (e.g. the map, various configs)
 * included incidential with the game.
 */
class ConfigParser
{
public:
    static Config parseConfig(const ParseResult& parse);
};
