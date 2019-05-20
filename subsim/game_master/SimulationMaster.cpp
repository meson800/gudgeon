#include "SimulationMaster.h"
#include <sstream>

#include "../common/Log.h"

SimulationMaster::SimulationMaster(Network* network_)
    : shouldShutdown(false)
    , network(network_)
    , EventReceiver({dispatchEvent<SimulationMaster, SimulationStartServer, &SimulationMaster::simStart>})
{
    lobbyInit = std::unique_ptr<LobbyHandler>(new LobbyHandler());
    network->registerCallback(lobbyInit.get());

}

SimulationMaster::~SimulationMaster()
{
    Log::writeToLog(Log::INFO, "Simulation master shutting down the simulation thread...");
    shouldShutdown = true;
    if (simLoop.joinable())
    {
        simLoop.join();
    }
    Log::writeToLog(Log::INFO, "Simulation thread shutdown successfully.");
}

void SimulationMaster::runSimLoop()
{
    Log::writeToLog(Log::INFO, "Main simulation loop started!");

    while (!shouldShutdown)
    {
        //TODO: possibly replace with a sleep until to keep game logic on a schedule?
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

HandleResult SimulationMaster::simStart(SimulationStartServer* event)
{
    assignments = event->assignments;

    std::ostringstream sstream;
    for (auto& teamPair : assignments)
    {
        sstream << "Team " << teamPair.first << ": {";
        for (auto& units : teamPair.second)
        {
            sstream << "{";
            for (auto& stationPair : units)
            {
                sstream << StationNames[stationPair.first] << "->" << stationPair.second << ", ";
            }
            sstream << "}";
        }
        sstream << "}";
    }
            

    Log::writeToLog(Log::INFO, "Starting server-side simulation. Final assignments:", sstream.str());
    // Unhook the lobby handler and destroy it
    network->deregisterCallback(lobbyInit.get());
    lobbyInit.reset();

    // Start the game loop
    Log::writeToLog(Log::L_DEBUG, "Simulation master attempting to start simulation thread...");
    simLoop = std::thread(&SimulationMaster::runSimLoop, this);

    return HandleResult::Stop;
}
