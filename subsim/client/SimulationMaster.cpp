#include "SimulationMaster.h"

#include "MockUIEvents.h"

#include <sstream>

#include "../common/Log.h"

SimulationMaster::SimulationMaster(Network* network_)
    : network(network_)
    , EventReceiver({
        dispatchEvent<SimulationMaster, SimulationStart, &SimulationMaster::simStart>,
        dispatchEvent<SimulationMaster, ConfigEvent, &SimulationMaster::configData>,
        })
{
    voiceHandler.reset(new VoiceHandler);
}

HandleResult SimulationMaster::simStart(SimulationStart* event)
{
    stations = event->stations;
    teamNames = event->teamNames;

    TeamOwnership team;
    for (auto station : stations)
    {
        team.team = station.team;
    }
    EventSystem::getGlobalInstance()->queueEvent(team);



    Log::writeToLog(Log::INFO, "Simulation started; closing lobby.");
    // Unhook the lobby handler and destroy it
    //ugly hack to destroy without getting into mutex land
    std::thread lobbyThread = std::thread(&SimulationMaster::destroyLobby, this);
    lobbyThread.detach();

    // again, hack because we can't add new callbacks inside a callback
    std::thread createThread = std::thread(&SimulationMaster::createStations, this);
    createThread.detach();


    return HandleResult::Stop;
}

HandleResult SimulationMaster::configData(ConfigEvent* event)
{
    Log::writeToLog(Log::L_DEBUG, "Received config data with terrain of size (",
        event->config.terrain.width, " x ", event->config.terrain.height, ")");
    config = event->config;
    return HandleResult::Stop;
}

void SimulationMaster::createStations()
{
    for (auto station : stations)
    {
        if (station.station == StationType::Tactical)
        {
            tactical.push_back(
                std::shared_ptr<TacticalStation>(new TacticalStation(station.team, station.unit, &config, teamNames)));
        }

        if (station.station == StationType::Helm)
        {
            helm.push_back(std::shared_ptr<HelmStation>(new HelmStation(station.team, station.unit, teamNames)));
        }
    }

    arduinoHandler.reset(new ArduinoHandler(stations.front().team, stations.front().unit));
}

bool SimulationMaster::ConnectionEstablished(RakNet::RakNetGUID other)
{
    Log::writeToLog(Log::INFO, "Connected to server ", other, "! Attempting to join lobby");
    // Register the join lobby callback now
    lobbyInit = std::unique_ptr<LobbyHandler>(new LobbyHandler());
    network->registerCallback(lobbyInit.get());
    lobbyInit->joinLobby(other, 1);

    // start the theme!
    EventSystem::getGlobalInstance()->queueEvent(ThemeAudio());

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
