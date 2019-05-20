#pragma once

#include "../common/EventSystem.h"
#include "../common/SimulationEvents.h"

#include "UI.h"
#include "MockUIEvents.h"

#include <queue>

/*!
 * Class that handles the tactical station. This includes "shitty IRC",
 * sonar, weapons, and repair
 */
class TacticalStation : public EventReceiver, public Renderable
{
public:
    /// Init internal state
    TacticalStation(); 

    /// Receives a text message from the server
    HandleResult receiveTextMessage(TextMessage* message);

    /// Handles key events from SDL. This enables/disables text input using the enter key
    HandleResult handleKeypress(KeyEvent* keypress);

    /// Handles text inputs from SDL. This is how we accumulate our shitty IRC message
    HandleResult handleText(TextInputEvent* text);

    /// Gets our current UnitState
    HandleResult handleUnitState(UnitState* state);
    
    /// Handles drawing the current state on the renderer.
    virtual void redraw() override;

private:
    /// Container for the last received text messages
    std::queue<std::string> lastMessages;

    /// Stores if we are receiving text
    bool receivingText;
};

