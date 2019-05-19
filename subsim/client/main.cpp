#include <iostream>

#include "version.h"

#include "../common/EventSystem.h"
#include "../common/Network.h"
#include "../common/Log.h"

#include "UI.h"
#include "SimulationMaster.h"


void print_usage(char* prog_name)
{
    Log::writeToLog(Log::FATAL, "Invalid command line arguments");
    std::cerr << prog_name << " -s [ip/hostname]\n";
}

int main(int argc, char **argv)
{
    Log::setLogfile(std::string(argv[0]) + ".log");
    Log::clearLog();
    Log::shouldMirrorToConsole(true);
    Log::setLogLevel(Log::ALL);

    Log::writeToLog(Log::INFO, "Subsim client version v", VERSION_MAJOR, ".", VERSION_MINOR, " starting");

    if (argc != 3 || std::string(argv[1]) != std::string("-s"))
    {
        print_usage(argv[0]);
        return 1;
    }

    // Startup the event system

    Network network;
    EventSystem events(&network);

    // Startup the UI
    UI ui;
    UI::setGlobalUI(&ui);

    // and start our JoinGame client
    SimulationMaster master(&network);
    network.registerCallback(&master);

    // Connect to get this party started
    network.connect(argv[2]);

    std::cout << "Press enter to exit...\n";
    std::string dummy;
    std::getline(std::cin, dummy);

    network.deregisterCallback(&master);
    return 0;
}
