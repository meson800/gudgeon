#include "LobbyHandler.h"

#include "../common/Log.h"
#include "../common/Exceptions.h"

#include "UI.h"

#include <SDL2_gfxPrimitives.h>

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
        renderer = UI::getGlobalUI()->getFreeRenderer(WIDTH, HEIGHT);
    }

    /* Unwrap the lobby status into stations per team */
    std::map<uint16_t, std::vector<std::pair<StationType, RakNet::RakNetGUID>>> perTeamLobby;
    for (auto station : status.stations)
    {
        auto idPair = station.first;
        RakNet::RakNetGUID system = station.second;

        // Make sure "free" stations didn't get assigned by accident
        if (idPair.second == StationType::Unassigned)
        {
            Log::writeToLog(Log::ERR, "Invalid lobby status from server! Station on team:", idPair.first, " has unassigned station type!");
            throw NetworkMessageError("Invalid lobby status: Unassigned station present in list!");
        }

        Log::writeToLog(Log::L_DEBUG, "Reading lobby status: <Team ", idPair.first, ": ", StationNames[idPair.second], " Station>: System ", station.second);
        perTeamLobby[idPair.first].push_back(std::pair<StationType, RakNet::RakNetGUID>(idPair.second, station.second));
    }

    // Now draw the lobby status, using grayed out circles for free, open spots
    uint16_t numTeams = perTeamLobby.size();

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (auto team : perTeamLobby)
    {
        uint16_t y = 30;

        // Space out teams evenly along the viewport
        uint16_t x = static_cast<double>(team.first + 1) / numTeams * WIDTH;

        Log::writeToLog(Log::L_DEBUG, "Drawing status of team ", team.first);
        for (auto station : team.second)
        {
            boxColor(renderer, x, y, x + 30, y + 30, 0xFFFFFFFF);
            if (filledCircleColor(renderer, x, y, 15, 0xFFFFFFFF) != 0)
            {
                Log::writeToLog(Log::ERR, "Unable to draw on renderer! SDL error:", SDL_GetError());
                throw std::runtime_error("Unable to draw using filledCircleColor");
            }
            SDL_RenderDrawPoint(renderer, x, y);
            y += 30;
        }
    }

    SDL_RenderPresent(renderer);
    SDL_RenderPresent(renderer);
        
    return true;
}
