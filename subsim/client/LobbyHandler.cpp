#include "LobbyHandler.h"

#include "../common/Log.h"
#include "../common/Exceptions.h"

#include "UI.h"
#include "MockUIEvents.h"

#include <SDL2_gfxPrimitives.h>
#include <sstream>

#define WIDTH 640
#define HEIGHT 480

LobbyHandler::LobbyHandler()
    : Renderable(WIDTH, HEIGHT)
    , EventReceiver(dispatchEvent<LobbyHandler, KeyEvent, &LobbyHandler::getKeypress>)
{
    Log::writeToLog(Log::L_DEBUG, "LobbyHandler started");
}

void LobbyHandler::joinLobby(RakNet::RakNetGUID server, uint8_t numStations)
{
    // Setup our state, initially with no requests.
    state.stations.clear();

    // Send along our message
    network->sendMessage(server, &state, PacketReliability::RELIABLE_ORDERED);
}

HandleResult LobbyHandler::getKeypress(KeyEvent* event)
{
    if (event->isDown || 
        (event->key != Key::Left && event->key != Key::Right && event->key != Key::Up 
        && event->key != Key::Enter))
    {
        Log::writeToLog(Log::L_DEBUG, "Skipping event with keypress:", event->key);
        return HandleResult::Continue;
    }
    
    Log::writeToLog(Log::L_DEBUG, "Got keyup press:", event->key);
    return HandleResult::Stop;
}


bool LobbyHandler::LobbyStatusRequested(RakNet::RakNetGUID other, const LobbyStatusRequest& request)
{
    return false;
}

bool LobbyHandler::UpdatedLobbyStatus(const LobbyStatus& status)
{
    unpackedState = status.stations;

    // Now draw the lobby status, using grayed out circles for free, open spots
    scheduleRedraw();

    return true;
}

void LobbyHandler::redraw()
{
    uint16_t numTeams = unpackedState.size();

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    uint64_t unassigned_color = 0xFF0000FF;
    uint64_t unconfirmed_color = 0xFCE205FF;
    uint64_t confirmed_color = 0x00FF00FF;



    SDL_RenderClear(renderer);

    int i = 0;
    for (auto teamPair : unpackedState)
    {
        uint16_t y = 30;

        // Space teams along viewport
        uint16_t x = (static_cast<double>(i) / numTeams * (WIDTH - 100) + 50);

        std::ostringstream sstream;
        sstream << "Team " << teamPair.second.first;
        drawText(sstream.str(), 25, x, y);

        for (auto unitPair : teamPair.second.second)
        {
            y += 30;
            drawText(std::string(unitPair.first), 20, x, y);

            for (auto stationPair : unitPair.second)
            {
                y += 25;
                drawText(StationNames[stationPair.first], 20, x + 20, y);
            }
        }
        ++i;
    }
        

    SDL_RenderPresent(renderer);
    SDL_RenderPresent(renderer);
}
