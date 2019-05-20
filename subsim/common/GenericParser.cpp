#include "GenericParser.h"

#include <sstream>
#include <iterator>

#include "Exceptions.h"
#include "Log.h"

ParseResult GenericParser::parse(const std::string& filename)
{
    std::ifstream infile(filename);
    if (infile.fail()) {
      throw GenericParseError("GenericParser failed to open file '" +
        filename + "'. Are you running the executable from the wrong "
        "directory?");
    }
    return parse(infile);
}

ParseResult GenericParser::parse(std::istream& instream)
{
    ParseResult result;

    bool inSection = false;
    std::string line;

    std::string sectionToken;
    ParseResult::iterator sectionIt;

    std::size_t lineNumber = 0;

    // Iterate line for line over the input file. If we aren't in a section,
    // then try to read a section. If we are in a section, try to read a key/value pair
    try 
    {
    while (std::getline(instream, line))
    {
        ++lineNumber;

        // Strip trailing whitespace, leading whitespace, and anything after and including a '#' character
        std::size_t commentStart = line.find_first_of('#');
        if (commentStart != std::string::npos)
        {
            line = line.substr(0, commentStart);
        }

        std::istringstream ss(line);
        // Read tokens out of the stringstream
        std::vector<std::string> tokens{std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>()};

        // Skip this line if it contains no tokens
        if (tokens.size() == 0)
        {
            continue;
        }

        // See if the first token contains an '=' character. If so, split into three tokens (means whitespace is optional around the equals)
        std::size_t equalsLocation = tokens[0].find_first_of('=');
        if (equalsLocation != std::string::npos)
        {
            std::string presplit = tokens[0];
            tokens[0] = presplit.substr(0, equalsLocation);
            tokens.insert(tokens.begin() + 1, presplit.substr(equalsLocation, 1));
            tokens.insert(tokens.begin() + 2, presplit.substr(equalsLocation + 1));
        }

        if (inSection)
        {
            if (tokens.size() == 2 && tokens[0] == "END" && tokens[1] == sectionToken)
            {
                // We found the end token. Abort!
                inSection = false;
                continue;
            }

            if (tokens.size() < 3) 
            {
                throw GenericParseError("Invalid number of tokens expected in the key/value entry!");
            }

            if (tokens[1] != "=")
            {
                throw GenericParseError("Second token is not an equals sign! Unexpected key/value entry!");
            }
            //otherwise, save this key/value entry
            std::vector<std::string> valueTokens{tokens.begin() + 2, tokens.end()};
            std::pair<std::string, std::vector<std::string>> toInsert(tokens[0], valueTokens);
            sectionIt->second.insert(toInsert);
        } else {
            if (tokens.size() != 2)
            {
                throw GenericParseError("Expected 2 tokens to begin section, got unexpected line.");
            }

            if (tokens[0] != "BEGIN")
            {
                throw GenericParseError("Unexpected section start!");
            }

            sectionToken = tokens[1];
            inSection = true;
            std::pair<std::string, ParseKVStore> toInsert(tokens[1], ParseKVStore());
            sectionIt = result.insert(toInsert);
        }
    }
    }

    catch (GenericParseError err)
    {
        Log::writeToLog(Log::ERR, "Parse error on line ", lineNumber, ":", line);
        // Rethrow for upstream consumers
        throw err;
    }
    return result;
}
