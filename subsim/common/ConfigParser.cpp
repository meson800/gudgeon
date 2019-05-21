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

            if (key == "terrain")
            {
                if (values.size() != 1)
                {
                    Log::writeToLog(Log::ERR, "Unexpected number of values in terrain config key! Number of values:", values.size());
                    throw ConfigParseError("Invalid number of values in terrain config");
                }

                unsigned pngResult = lodepng::decode(result.terrain.map, result.terrain.width, result.terrain.height,
                    values[0], LodePNGColorType::LCT_GREY, 8);

                if (pngResult != 0)
                {
                    Log::writeToLog(Log::ERR, "LodePNG load error:", lodepng_error_text(pngResult));
                    throw ConfigParseError("Invalid PNG file loaded as terrain.");
                }
            }

            if (key == "terrain_scale")
            {
                if (values.size() != 1)
                {
                    Log::writeToLog(Log::ERR, "Unexpected number of values in terrain scale config key! Number of values:", values.size());
                    throw ConfigParseError("Invalid number of values in terrain scale key");
                }
                std::istringstream sstream(values[0]);

                sstream >> result.terrain.scale;
            }
        }
    }
    return result;
}
