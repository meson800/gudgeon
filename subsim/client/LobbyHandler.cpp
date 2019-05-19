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
    , selectedTeam(1)
    , selectedUnit(0)
    , selectedStation(0)
{
    Log::writeToLog(Log::L_DEBUG, "LobbyHandler started");
}

LobbyHandler::~LobbyHandler()
{
    Log::writeToLog(Log::L_DEBUG, "LobbyHandler ", this, " shutting down.");
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
        && event->key != Key::Down && event->key != Key::Enter))
    {
        Log::writeToLog(Log::L_DEBUG, "Skipping event with keypress:", event->key);
        return HandleResult::Continue;
    }
    
    Log::writeToLog(Log::L_DEBUG, "Got keyup press:", event->key);
    {
        std::lock_guard<std::mutex> guard(mux);
        
        switch (event->key)
        {
            case Key::Left:
            {
                // Go to the previous team
                auto it = unpackedState.find(selectedTeam);
                if (it == unpackedState.end())
                {
                    Log::writeToLog(Log::ERR, "Cursor is on invalid team:", selectedTeam);
                    throw LobbyError("Invalid cursor team location!");
                }

                if (it != unpackedState.begin())
                {
                    --it;
                    selectedTeam = it->first;
                    selectedUnit = 0;
                    selectedStation = 0;
                }
            }
            break;

            case Key::Right:
            {
                // go to the next team
                auto it = unpackedState.find(selectedTeam);
                if (it == unpackedState.end())
                {
                    Log::writeToLog(Log::ERR, "Cursor is on invalid team:", selectedTeam);
                    throw LobbyError("Invalid cursor team location!");
                }

                ++it;
                if (it != unpackedState.end())
                {
                    selectedTeam = it->first;
                    selectedUnit = 0;
                    selectedStation = 0;
                }
            }
            break;

            case Key::Down:
            {
                // Go to the next station
                uint32_t numUnits = unpackedState[selectedTeam].second.size();
                uint32_t numStations = unpackedState[selectedTeam].second[selectedUnit].second.size();
                if (selectedStation < numStations - 1)
                {
                    ++selectedStation;
                    break;
                }

                // Otherwise, go to the next unit
                if (selectedUnit < numUnits - 1)
                {
                    ++selectedUnit;
                    selectedStation = 0;
                    break;
                }
            }
            break;

            case Key::Up:
            {
                // Go up to the next station
                uint32_t numUnits = unpackedState[selectedTeam].second.size();
                uint32_t numStations = unpackedState[selectedTeam].second[selectedUnit].second.size();

                if (selectedStation > 0)
                {
                    --selectedStation;
                    break;
                }

                // Otherwise, go to the previous unit
                if (selectedUnit > 0)
                {
                    --selectedUnit;
                    selectedStation = unpackedState[selectedTeam].second[selectedUnit].second.size() - 1;
                    break;
                }
            }
            break;

            case Key::Enter:
            {
                LobbyStatusRequest request;
                LobbyStatusRequest::StationID id;
                id.team = selectedTeam;
                id.unit = selectedUnit;
                id.station = selectedStation;

                bool should_assign = unpackedState[selectedTeam].second[selectedUnit].second[selectedStation].second != network->getOurGUID();

                request.stations.push_back(std::pair<LobbyStatusRequest::StationID, bool>(id, should_assign));
                network->sendMessage(network->getFirstConnectionGUID(), &request, PacketReliability::RELIABLE_SEQUENCED);
            }
                
        }
    }
    scheduleRedraw();

    return HandleResult::Stop;
}


bool LobbyHandler::LobbyStatusRequested(RakNet::RakNetGUID other, const LobbyStatusRequest& request)
{
    return false;
}

bool LobbyHandler::UpdatedLobbyStatus(const LobbyStatus& status)
{
    {
        std::lock_guard<std::mutex> guard(mux);
        unpackedState = status.stations;
    }

    // Now draw the lobby status
    scheduleRedraw();

    return true;
}

constexpr uint32_t rgba_to_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | ((uint32_t)r);
}

void LobbyHandler::redraw()
{
    std::lock_guard<std::mutex> guard(mux);

    uint16_t numTeams = unpackedState.size();

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    uint32_t unassigned_color = rgba_to_color(0xFF, 0, 0, 0xFF);
    uint32_t other_color = rgba_to_color(0xFC, 0xE2, 0x05, 0xFF);
    uint32_t us_color = rgba_to_color(0, 0xFF, 0, 0xFF);
    uint32_t us_hover_color = rgba_to_color(0, 0, 0xBB, 0xFF);


    SDL_RenderClear(renderer);

    int i = 0;
    for (auto teamPair : unpackedState)
    {
        uint16_t team = teamPair.first;
        uint16_t y = 30;

        // Space teams along viewport
        uint16_t x = (static_cast<double>(i) / numTeams * (WIDTH - 100) + 50);

        std::ostringstream sstream;
        sstream << "Team " << teamPair.second.first;
        drawText(sstream.str(), 25, x, y);

        uint16_t unit = 0;

        for (auto unitPair : teamPair.second.second)
        {
            y += 30;
            drawText(std::string(unitPair.first), 20, x, y);

            uint16_t station = 0;
            for (auto stationPair : unitPair.second)
            {
                y += 25;

                uint64_t color;
                if (stationPair.second == RakNet::UNASSIGNED_RAKNET_GUID)
                {
                    color = unassigned_color;
                } else if (stationPair.second != network->getOurGUID()) {
                    color = other_color;
                } else {
                    color = us_color;
                }

                if (selectedTeam == team && selectedUnit == unit && selectedStation == station)
                {
                    filledCircleColor(renderer, x + 5, y + 15, 13, us_hover_color);
                }

                filledCircleColor(renderer, x + 5, y + 15, 10, color);
                drawText(StationNames[stationPair.first], 20, x + 20, y);

                ++station;
            }

            ++unit;
        }
        ++i;
    }
        

    SDL_RenderPresent(renderer);
    SDL_RenderPresent(renderer);
}
