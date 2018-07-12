#include <iostream>

#include "version.h"

#include "../common/Network.h"
#include "../common/Log.h"

#include "UI.h"
#include "LobbyHandler.h"


/**
 * Top level recieving class that tries to join a lobby upon connect.
 * Once the lobby has been joined, it attempts to start a game as soon as
 * the lobby is ready
 */
class JoinGame : public ReceiveInterface
{
private:
    LobbyHandler lobby;

public:
    virtual bool ConnectionEstablished(RakNet::RakNetGUID other) override
    {
        Log::writeToLog(Log::INFO, "Connected to server ", other, "! Attempting to join lobby");
        // Register the join lobby callback now
        network->registerCallback(&lobby);

        lobby.joinLobby(other, 1);
        return true;
    }

    virtual bool ConnectionLost(RakNet::RakNetGUID other) override
    {
        Log::writeToLog(Log::INFO, "Lost connection with server ", other, ". Shutting down lobby");
        network->deregisterCallback(&lobby);
        return true;
    }
};

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

    Network network;

    // Startup the UI
    UI ui;
    UI::setGlobalUI(&ui);

    // and start our JoinGame client
    JoinGame game;
    network.registerCallback(&game);

    // Connect to get this party started
    network.connect(argv[2]);

    std::cout << "Press enter to exit...\n";
    std::string dummy;
    std::getline(std::cin, dummy);

    network.deregisterCallback(&game);
    return 0;
}
