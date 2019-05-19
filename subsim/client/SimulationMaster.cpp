#include "SimulationMaster.h"
#include <sstream>

#include "../common/Log.h"

SimulationMaster::SimulationMaster(Network* network_)
    : network(network_)
    , EventReceiver(dispatchEvent<SimulationMaster, SimulationStart, &SimulationMaster::simStart>)
{
}

HandleResult SimulationMaster::simStart(SimulationStart* event)
{
    stations = event->stations;

    Log::writeToLog(Log::INFO, "Simulation started; closing lobby.");
    // Unhook the lobby handler and destroy it
    //ugly hack to destroy without getting into mutex land
    std::thread lobbyThread = std::thread(&SimulationMaster::destroyLobby, this);
    lobbyThread.detach();

    return HandleResult::Stop;
}

bool SimulationMaster::ConnectionEstablished(RakNet::RakNetGUID other)
{
    Log::writeToLog(Log::INFO, "Connected to server ", other, "! Attempting to join lobby");
    // Register the join lobby callback now
    lobbyInit = std::unique_ptr<LobbyHandler>(new LobbyHandler());
    network->registerCallback(lobbyInit.get());
    lobbyInit->joinLobby(other, 1);

    return true;
}

bool SimulationMaster::ConnectionLost(RakNet::RakNetGUID other)
{
    // Destroy the lobby if it exists
    if (lobbyInit)
    {
        //ugly hack to destroy without getting into mutex land
        std::thread lobbyThread = std::thread(&SimulationMaster::destroyLobby, this);
        lobbyThread.detach();
    }
    return true;
}

void SimulationMaster::destroyLobby()
{
    network->deregisterCallback(lobbyInit.get());
    lobbyInit.reset();
}
