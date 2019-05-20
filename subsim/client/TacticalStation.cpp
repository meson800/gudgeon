#include "TacticalStation.h"

#include "UI.h"
#include <SDL2_gfxPrimitives.h>
#include "../common/Log.h"

#define WIDTH 640
#define HEIGHT 480

TacticalStation::TacticalStation(uint32_t team, uint32_t unit)
    : Renderable(WIDTH, HEIGHT)
    , EventReceiver({
        dispatchEvent<TacticalStation, KeyEvent, &TacticalStation::handleKeypress>,
        dispatchEvent<TacticalStation, TextInputEvent, &TacticalStation::handleText>,
        dispatchEvent<TacticalStation, TextMessage, &TacticalStation::receiveTextMessage>,
        dispatchEvent<TacticalStation, UnitState, &TacticalStation::handleUnitState>,
        dispatchEvent<TacticalStation, SonarDisplayState, &TacticalStation::handleSonarDisplay>
    })
    , team(team)
    , unit(unit)
    , receivingText(false)
{}

HandleResult TacticalStation::handleKeypress(KeyEvent* keypress)
{
    if (keypress->isDown == false && keypress->key == Key::Enter)
    {
        // Swap text input status
        IgnoreKeypresses ignore;
        receivingText = !receivingText;
        ignore.shouldIgnore = receivingText;
        // Inform other modules that we are receiving text
        EventSystem::getGlobalInstance()->queueEvent(ignore);

        // And update the UI
        UI::getGlobalUI()->changeTextInput(receivingText);
    }
    return HandleResult::Stop;
}

HandleResult TacticalStation::handleText(TextInputEvent* text)
{
    Log::writeToLog(Log::L_DEBUG, "Received TextInput from the server. Text:", text->text);
    return HandleResult::Stop;
}

HandleResult TacticalStation::receiveTextMessage(TextMessage* message)
{
    Log::writeToLog(Log::L_DEBUG, "Received TextMessage from the server. Message:", message->message);

    return HandleResult::Stop;
}

HandleResult TacticalStation::handleUnitState(UnitState* state)
{
    if (state->team == team && state->unit == unit) {
        Log::writeToLog(Log::L_DEBUG, "Got updated UnitState from server");
        lastState = *state;
        scheduleRedraw();
    }
    return HandleResult::Stop;
}

HandleResult TacticalStation::handleSonarDisplay(SonarDisplayState* sonar)
{
    Log::writeToLog(Log::L_DEBUG, "Got updated SonarDisplay from server");
    lastSonar = *sonar;
    scheduleRedraw();
    return HandleResult::Stop;
}

void TacticalStation::redraw()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw a grid. We want to give the impression of an infinite grid without
    // actually drawing infinitely many lines. So we actually just draw the part
    // of the grid that's at the unit's position plus or minus WIDTH.
    constexpr int grid_size = 100;
    int64_t grid_min_x = ((lastState.x - WIDTH) / grid_size) * grid_size;
    int64_t grid_max_x = ((lastState.x + WIDTH) / grid_size) * grid_size;
    int64_t grid_min_y = ((lastState.y - WIDTH) / grid_size) * grid_size;
    int64_t grid_max_y = ((lastState.y + WIDTH) / grid_size) * grid_size;
    for (int64_t x = grid_min_x; x < grid_max_x; x += grid_size) {
        lineRGBA(renderer,
            display_x(x, grid_min_y), display_y(x, grid_min_y),
            display_x(x, grid_max_y), display_y(x, grid_max_y),
            100, 100, 100, 255);
    }
    for (int64_t y = grid_min_y; y < grid_max_y; y += grid_size) {
        lineRGBA(renderer,
            display_x(grid_min_x, y), display_y(grid_min_x, y),
            display_x(grid_max_x, y), display_y(grid_max_x, y),
            100, 100, 100, 255);
    }

    for (const SonarDisplayState::Dot &dot : lastSonar.dots) {
        circleRGBA(renderer, display_x(dot.x, dot.y), display_y(dot.x, dot.y), 30, 255, 0, 0, 255);
    }

    SDL_RenderPresent(renderer);
}

int64_t TacticalStation::display_x(int64_t x, int64_t y) {
    return WIDTH/2
        - (x - lastState.x) * sin(lastState.heading * 2*M_PI/360.0)
        + (y - lastState.y) * cos(lastState.heading * 2*M_PI/360.0);
}

int64_t TacticalStation::display_y(int64_t x, int64_t y) {
    return HEIGHT/2
        - (x - lastState.x) * cos(lastState.heading * 2*M_PI/360.0)
        - (y - lastState.y) * sin(lastState.heading * 2*M_PI/360.0);
}

