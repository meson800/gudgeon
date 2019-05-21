#include <iostream>
#include <string>
#include <thread>

#include "version.h"
#include "../common/EventSystem.h"
#include "../common/Network.h"
#include "../common/Log.h"

#include "SimulationMaster.h"

void print_usage(char* prog_name)
{
    Log::writeToLog(Log::FATAL, "Invalid command line arguments");
    std::cerr << prog_name << " -f [config_file]\n";
}

int main(int argc, char **argv)
{
    Log::setLogfile(std::string(argv[0]) + ".log");
    Log::clearLog();
    Log::shouldMirrorToConsole(true);
    Log::setLogLevel(Log::ALL);

    Log::writeToLog(Log::INFO, "Subsim game master version v", VERSION_MAJOR, ".", VERSION_MINOR, " started");

    if (argc != 3 || std::string(argv[1]) != std::string("-f"))
    {
        print_usage(argv[0]);
        return 1;
    }


    Network network(true);
    EventSystem events(&network);

    SimulationMaster master(&network, argv[2]);

    std::cout << "Press enter to exit...\n";
    std::string dummy;
    std::getline(std::cin, dummy);
    return 0;
}
