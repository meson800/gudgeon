#pragma once

#include "../common/EventSystem.h"
#include "../common/SimulationEvents.h"

#include "UI.h"
#include "MockUIEvents.h"

#include <queue>

/// forward declaration
class Terrain;

/*!
 * Class that handles the tactical station. This includes "shitty IRC",
 * sonar, weapons, and repair
 */
class TacticalStation : public EventReceiver, public Renderable
{
public:
    /// Init internal state, with team/unit plus pointer to a terrain object
    TacticalStation(uint32_t team, uint32_t unit, Terrain* terrain_);

    /// Receives a text message from the server
    HandleResult receiveTextMessage(TextMessage* message);

    /// Handles key events from SDL. This enables/disables text input using the enter key
    HandleResult handleKeypress(KeyEvent* keypress);

    /// Handles text inputs from SDL. This is how we accumulate our shitty IRC message
    HandleResult handleText(TextInputEvent* text);

    /// Gets our current UnitState
    HandleResult handleUnitState(UnitState* state);

    /// Gets our current sonar display state
    HandleResult handleSonarDisplay(SonarDisplayState* sonar);

    /// Handles drawing the current state on the renderer.
    virtual void redraw() override;

private:
    /// Renders the tube status in the upper right corner
    void renderTubeState();

    /// Renders the terrain
    void renderSDTerrain();

    /// Renders a submarine on the sonar screen
    void renderSDSubmarine(int64_t x, int64_t y, int16_t heading);

    /// Render various geometric primitives on the sonar screen. These are
    /// analogous to the corresponding SDL display primitives, but they convert
    /// to sonar display coordinates automatically.
    void renderSDCircle(int64_t x, int64_t y, int16_t r, uint32_t color);
    void renderSDLine(int64_t x1, int64_t y1, int64_t x2, int64_t y2, uint32_t color);
    void renderSDArc(int64_t x, int64_t y, int16_t r, int16_t a1, int16_t a2, uint32_t color);
    void renderSDFilledPolygon(const int64_t *xs, const int64_t *ys, int count, uint32_t color);

    /// sdX() and sdY() convert from world coordinates to SDL display
    /// coordinates based on the unit's current location
    int64_t sdX(int64_t x, int64_t y);
    int64_t sdY(int64_t x, int64_t y);

    // sdHeading() converts from a global heading to a SDL angle number
    int16_t sdHeading(int16_t heading);

    uint32_t team;
    uint32_t unit;

    /// Container for the last received text messages
    std::queue<std::string> lastMessages;

    /// Stores if we are receiving text
    bool receivingText;
    
    /// Last received unit state for our unit
    UnitState lastState;
    
    /// Last received sonar state (shared across all units)
    SonarDisplayState lastSonar;

    /// Stores a pointer to the Terrain object we will use
    Terrain* terrain;
};

