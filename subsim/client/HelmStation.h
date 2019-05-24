#pragma once

#include "../common/EventSystem.h"
#include "../common/SimulationEvents.h"

#include "UI.h"
#include "MockUIEvents.h"

#include <queue>

/*!
 * Class that handles the helm station. This does steering.
 * sonar, weapons, and repair
 */
class HelmStation : public EventReceiver
{
public:
    /// Init internal state
    HelmStation(uint32_t team_, uint32_t unit_, std::map<uint32_t, std::string> teamNames_); 

    /// Handles key events from SDL. This enables/disables text input using the enter key
    HandleResult handleKeypress(KeyEvent* keypress);

    /// Gets our current UnitState
    HandleResult handleUnitState(UnitState* state);

    /// Handles the case where we should ignore mocks for keyboard input
    HandleResult handleMockIgnore(IgnoreKeypresses* event);

    /// Handles a score update
    HandleResult handleScore(ScoreEvent* event);

private:
    /// Stores the last received UnitState
    UnitState lastState; 

    /// Stores the last score update we got
    std::map<uint32_t, uint32_t> scores;

    /// Stores the team names
    std::map<uint32_t, std::string> teamNames;

    /// Stores if we are ignoring key input because text entry is happening
    bool ignoringMocks;

    /// Stores which unit we are controlling
    uint32_t team, unit;
};

