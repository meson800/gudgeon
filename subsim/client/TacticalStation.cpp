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

    for (const SonarDisplayState::Dot &dot : lastSonar.dots) {
        circleRGBA(renderer, dot.x, dot.y, 30, 255, 0, 0, 255);
    }

    SDL_RenderPresent(renderer);
}
