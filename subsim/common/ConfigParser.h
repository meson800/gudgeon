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
    bool wallAt(int32_t tx, int32_t ty) const
    {
        if (tx < 0 || tx >= width || ty < 0 || ty >= height) {
            return true;
        }
        return (map[tx+(height-1-ty)*width] < 255);
    }
};

class Config
{
public:
    Terrain terrain;

    uint16_t subTurningSpeed;
    uint16_t subMaxSpeed;

    uint16_t sonarRange;

    uint16_t torpedoSpread;
    uint16_t torpedoSpeed;
    uint16_t collisionRadius;

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
