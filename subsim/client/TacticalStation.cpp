#include "TacticalStation.h"

#include "../common/Messages.h"

#include "../common/Log.h"

#include "UI.h"
#include <SDL2_gfxPrimitives.h>

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
    if (receivingText)
    {
        // we're in text entry mode, escape!
        return HandleResult::Unhandled;
    }

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

    if (keypress->isDown == false && keypress->key == Key::Space)
    {
        FireEvent fire;
        fire.team = team;
        fire.unit = unit;

        EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(fire));
        Log::writeToLog(Log::L_DEBUG, "Fired torpedos/mines");
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
    constexpr int gridSize = 100;
    int64_t gridMinX = ((lastState.x - WIDTH) / gridSize) * gridSize;
    int64_t gridMaxX = ((lastState.x + WIDTH) / gridSize) * gridSize;
    int64_t gridMinY = ((lastState.y - WIDTH) / gridSize) * gridSize;
    int64_t gridMaxY = ((lastState.y + WIDTH) / gridSize) * gridSize;
    for (int64_t x = gridMinX; x < gridMaxX; x += gridSize)
    {
        lineRGBA(renderer,
            displayX(x, gridMinY), displayY(x, gridMinY),
            displayX(x, gridMaxY), displayY(x, gridMaxY),
            100, 100, 100, 255);
    }
    for (int64_t y = gridMinY; y < gridMaxY; y += gridSize)
    {
        lineRGBA(renderer,
            displayX(gridMinX, y), displayY(gridMinX, y),
            displayX(gridMaxX, y), displayY(gridMaxX, y),
            100, 100, 100, 255);
    }

    renderSubmarine(lastState.x, lastState.y, lastState.heading);

    for (const UnitSonarState &u : lastSonar.units)
    {
        if (u.team == team && u.unit == unit)
        {
            // Don't draw ourself this way
            continue;
        }
        circleRGBA(renderer, displayX(u.x, u.y), displayY(u.x, u.y), 30, 255, 0, 0, 255);
    }

    for (const TorpedoState &torp : lastSonar.torpedos)
    {
        circleRGBA(renderer, displayX(torp.x, torp.y), displayY(torp.x, torp.y), 10, 255, 0, 0, 255);
    }

    for (const MineState &mine : lastSonar.mines)
    {
        circleRGBA(renderer, displayX(mine.x, mine.y), displayY(mine.x, mine.y), 20, 255, 0, 0, 255);
    }

    SDL_RenderPresent(renderer);
}

void TacticalStation::renderSubmarine(int64_t x, int64_t y, int16_t heading) {
    float u = cos(heading * 2*M_PI/360.0);
    float v = sin(heading * 2*M_PI/360.0);
    arcRGBA(renderer,
        displayX(x+u*10, y+v*10), displayY(x+u*10, y+v*10),
        10,
        displayHeading(heading + 90), displayHeading(heading - 90),
        255, 0, 0, 255);
    arcRGBA(renderer,
        displayX(x-u*10, y-v*10), displayY(x-u*10, y-v*10),
        10,
        displayHeading(heading - 90), displayHeading(heading + 90),
        255, 0, 0, 255);
    lineRGBA(renderer,
        displayX(x+u*10+v*10, y+v*10-u*10), displayY(x+u*10+v*10, y+v*10-u*10),
        displayX(x-u*10+v*10, y-v*10-u*10), displayY(x-u*10+v*10, y-v*10-u*10),
        255, 0, 0, 255);
    lineRGBA(renderer,
        displayX(x+u*10-v*10, y+v*10+u*10), displayY(x+u*10-v*10, y+v*10+u*10),
        displayX(x-u*10-v*10, y-v*10+u*10), displayY(x-u*10-v*10, y-v*10+u*10),
        255, 0, 0, 255);
}

int64_t TacticalStation::displayX(int64_t x, int64_t y) {
    return WIDTH/2
        - (x - lastState.x) * sin(lastState.heading * 2*M_PI/360.0)
        + (y - lastState.y) * cos(lastState.heading * 2*M_PI/360.0);
}

int64_t TacticalStation::displayY(int64_t x, int64_t y) {
    return HEIGHT/2
        - (x - lastState.x) * cos(lastState.heading * 2*M_PI/360.0)
        - (y - lastState.y) * sin(lastState.heading * 2*M_PI/360.0);
}

int16_t TacticalStation::displayHeading(int16_t heading) {
    return heading - lastState.heading + 90;
}


