#pragma once

#include <cstdint>

#include "../common/Lobby.h"
#include "../common/Network.h"

/// Forward declaration of SDL_Renderer
class SDL_Renderer;

/*!
 * Client version of the LobbyHandler. Recieves LobbyStatus
 * and sends the server any local user-induced lobby changes to the server
 */
class LobbyHandler : public ReceiveInterface
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

private:
    /// Stores the renderer context
    SDL_Renderer* renderer;

    /// Stores our current requested station state
    LobbyStatusRequest state;
};
