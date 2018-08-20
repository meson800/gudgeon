#pragma once

#include "../common/Lobby.h"
#include "../common/Network.h"

/*!
 * Server version of the LobbyHandler. Manages the lobby and 
 * sends updates to all connected nodes that are also waiting on
 * lobby status.
 */
class LobbyHandler : public ReceiveInterface
{
public:
    /// Sets up the initial lobby status, by opening lobby.cfg
    LobbyHandler();
    /// Catch disconnect events so we can remove them from the lobby
    virtual bool ConnectionLost(RakNet::RakNetGUID other) override;
    /// Catch lobby request events so we can identify people who want to join the lobby
    virtual bool LobbyStatusRequested(RakNet::RakNetGUID other, const LobbyStatusRequest& request) override;
    virtual bool UpdatedLobbyStatus(const LobbyStatus& status) override;

private:
    /** 
     * Stores the GUIDs of systems currently interested in lobby status,
     * that is, the list of systems who have sent us LobbyStatus requests
     */
    std::set<RakNet::RakNetGUID> waitingSystems;

    /**
     * Stores the current LobbyStatus
     */
    LobbyStatus status;
};
