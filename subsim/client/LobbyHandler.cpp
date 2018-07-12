#include "LobbyHandler.h"

#include "UI.h"

#define WIDTH 640
#define HEIGHT 480

LobbyHandler::LobbyHandler()
    : renderer(nullptr)
{}

void LobbyHandler::joinLobby(RakNet::RakNetGUID server, uint8_t numStations)
{
    // Setup our state
    state.stations.clear();
    for (uint8_t i = 0; i < numStations; ++i)
    {
        state.stations.push_back(std::pair<uint16_t, StationType>(0, StationType::Unassigned));
    }
    // Send along our message
    network->sendMessage(server, &state, PacketReliability::RELIABLE_ORDERED);
}


bool LobbyHandler::LobbyStatusRequested(RakNet::RakNetGUID other, const LobbyStatusRequest& request)
{
    return false;
}

bool LobbyHandler::UpdatedLobbyStatus(const LobbyStatus& status)
{
    // Startup a renderer if we haven't already to visually display state
    if (renderer == nullptr)
    {
        UI::getGlobalUI()->getFreeRenderer(WIDTH, HEIGHT);
    }
        
    return true;
}
