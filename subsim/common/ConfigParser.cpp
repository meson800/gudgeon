#include "ConfigParser.h"
#include "Log.h"
#include "Exceptions.h"

#include "lodepng.h"

#include <sstream>

Config ConfigParser::parseConfig(const ParseResult& parse)
{
    Config result;
    result.terrain.width = 0;
    result.terrain.height = 0;
    result.terrain.scale = 1;
    auto range = parse.equal_range("CONFIG");
    for (auto it = range.first; it != range.second; ++it)
    {
        for (auto keyValIt = it->second.cbegin(); keyValIt != it->second.cend(); ++keyValIt)
        {
            const std::string& key = keyValIt->first;
            const std::vector<std::string>& values = keyValIt->second;

            if (values.size() != 1)
            {
                Log::writeToLog(Log::ERR, "Unexpected number of values in config key:", key,
                    "! Number of values:", values.size());
                throw ConfigParseError("Invalid number of values in config key");
            }

            if (key == "terrain")
            {
                std::vector<uint8_t> colorData;
                unsigned pngResult = lodepng::decode(colorData, result.terrain.width, result.terrain.height,
                    values[0], LodePNGColorType::LCT_RGB, 8);

                if (pngResult != 0)
                {
                    Log::writeToLog(Log::ERR, "LodePNG load error:", lodepng_error_text(pngResult));
                    throw ConfigParseError("Invalid PNG file loaded as terrain.");
                }

                for (uint32_t i = 0; i < result.terrain.width; ++i)
                {
                    for (uint32_t j = 0; j < result.terrain.height; ++j)
                    {
                        uint8_t r = colorData[3 * (i + j * result.terrain.width)];
                        uint8_t g = colorData[3 * (i + j * result.terrain.width) + 1];
                        uint8_t b = colorData[3 * (i + j * result.terrain.width) + 2];

                        // We have a terrain piece if the color is black
                        result.terrain.map.push_back(r == 0 && g == 0 && b == 0);

                        // Check if we have a team 1 starting location (red)
                        if (r == 255 && g == 0 && b == 0)
                        {
                            result.startLocations[0].push_back(std::pair<int64_t, int64_t>(i, j));
                        }

                        // Check if we have a team 2 starting location (blue)
                        if (r == 0 && g == 0 && b == 255)
                        {
                            result.startLocations[1].push_back(std::pair<int64_t, int64_t>(i, j));
                        }
                    }
                }
                        
            }

            std::istringstream sstream(values[0]);
            if (key == "terrain_scale")
            {
                sstream >> result.terrain.scale;
            }

            if (key == "sub_turning_speed")
            {
                sstream >> result.subTurningSpeed;
            }

            if (key == "sub_max_speed")
            {
                sstream >> result.subMaxSpeed;
            }

            if (key == "sonar_range")
            {
                sstream >> result.sonarRange;
            }

            if (key == "torpedo_spread")
            {
                sstream >> result.torpedoSpread;
            }

            if (key == "torpedo_speed")
            {
                sstream >> result.torpedoSpeed;
            }

            if (key == "collision_radius")
            {
                sstream >> result.collisionRadius;
            }

            if (key == "max_mines")
            {
                sstream >> result.maxMines;
            }

            if (key == "max_torpedos")
            {
                sstream >> result.maxTorpedos;
            }
        }
    }
    return result;
}
