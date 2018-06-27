#include <iostream>

#include "version.h"

#include "../common/Network.h"
#include "../common/Log.h"

void print_usage(char* prog_name)
{
    std::cerr << prog_name << " -s [ip/hostname]\n";
}

int main(int argc, char **argv)
{
    Log::setLogfile(std::string(argv[0]) + ".log");
    Log::clearLog();
    Log::shouldMirrorToConsole(true);
    Log::setLogLevel(Log::ALL);

    Log::writeToLog(Log::INFO, "Subsim client version v", VERSION_MAJOR, ".", VERSION_MINOR, " starting");

    if (argc != 3 && std::string(argv[1]) != std::string("-s"))
    {
        print_usage(argv[0]);
        return 1;
    }

    Network network;
    network.connect(argv[2]);
    return 0;
}
