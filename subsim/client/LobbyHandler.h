#pragma once

#include <cstdint>

#include <map>

#include "../common/Lobby.h"
#include "../common/Network.h"
#include "../common/EventSystem.h"
#include "UI.h"

/// Forward declaration of SDL_Renderer
class SDL_Renderer;

/// Forward declaration of the key event
class KeyEvent;

/*!
 * Client version of the LobbyHandler. Recieves LobbyStatus
 * and sends the server any local user-induced lobby changes to the server
 *
 * Inherits from both RecieveInterface and Renderable, so that we recieve callbacks for
 * network calls and can draw on the screen.
 */
class LobbyHandler : public ReceiveInterface, public Renderable, public EventReceiver
{
public:
    /// Inits our internal state
    LobbyHandler();

    /// Contacts the given server, requesting to join the lobby with a given number of stations
    void joinLobby(RakNet::RakNetGUID server, uint8_t numStations);

    /// Handles incoming lobby status requests (ignoring it)
    virtual bool LobbyStatusRequested(RakNet::RakNetGUID other, const LobbyStatusRequest& request) override;

    /// Handles incoming lobby status information, displaying it on the screen
    virtual bool UpdatedLobbyStatus(const LobbyStatus& status) override;

    /// Handles drawing the current state on the renderer.
    virtual void redraw() override;

private:
    /// Stores our current requested station state
    LobbyStatusRequest state;

    /// Stores the currently selected entry under our cursor
    uint16_t selectedTeam;
    uint16_t selectedUnit;
    uint16_t selectedStation;

    std::map<uint16_t, Team_owner_t> unpackedState;

    HandleResult getKeypress(KeyEvent* event);

    std::mutex mux;
};
