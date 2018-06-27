#include <iostream>

#include "version.h"
#include "../common/Network.h"
#include "../common/Log.h"

int main(int argc, char **argv)
{
    Log::setLogfile(std::string(argv[0]) + ".log");
    Log::clearLog();
    Log::shouldMirrorToConsole(true);
    Log::setLogLevel(Log::ALL);

    Log::writeToLog(Log::INFO, "Subsim game master version v", VERSION_MAJOR, ".", VERSION_MINOR, " started");

    Network network(true);

    return 0;
}
