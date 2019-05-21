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
                result.terrain.map.reserve(result.terrain.width * result.terrain.height);
                for (int i = 0; i < result.terrain.width * result.terrain.height; ++i)
                {
                    result.terrain.map.push_back(
                      (colorData[3*i + 0] << 24) +
                      (colorData[3*i + 1] << 16) +
                      (colorData[3*i + 2] << 8) +
                      0xFF);
                }

                for (uint32_t tx = 0; tx < result.terrain.width; ++tx)
                {
                    for (uint32_t ty = 0; ty < result.terrain.height; ++ty)
                    {
                        uint32_t color = result.terrain.colorAt(tx, ty);
                        if (color == Terrain::START1)
                        {
                            result.startLocations[1].push_back(std::pair<int64_t, int64_t>(tx, ty));
                        }
                        else if (color == Terrain::START2)
                        {
                            result.startLocations[2].push_back(std::pair<int64_t, int64_t>(tx, ty));
                        }
                        else if (color == Terrain::FLAG1)
                        {
                            result.flags[1].push_back(std::pair<int64_t, int64_t>(tx, ty));
                        } 
                        else if (color ==  Terrain::FLAG2)
                        {
                            result.flags[2].push_back(std::pair<int64_t, int64_t>(tx, ty));
                        }
                        else
                        {
                            if (color != Terrain::EMPTY && color != Terrain::WALL)
                            {
                                Log::writeToLog(Log::ERR,
                                    "Invalid pixel at (", tx, ",", ty, "): ",
                                    "color is r=", ((color >> 24) & 0xFF),
                                    " g=", ((color >> 16) & 0xFF),
                                    " b=", ((color >> 8) & 0xFF), ".");
                                throw ConfigParseError("Terrain PNG has unexpected color.");
                            }
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
