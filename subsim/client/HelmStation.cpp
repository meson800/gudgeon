#include "HelmStation.h"

#include "../common/Messages.h"

#include "../common/Log.h"

#include "UI.h"
#include <SDL2_gfxPrimitives.h>

#include <sstream>

#define WIDTH 640
#define HEIGHT 480

HelmStation::HelmStation(uint32_t team_, uint32_t unit_)
    : Renderable(WIDTH, HEIGHT)
    , EventReceiver({
        dispatchEvent<HelmStation, KeyEvent, &HelmStation::handleKeypress>,
        dispatchEvent<HelmStation, UnitState, &HelmStation::handleUnitState>,
        dispatchEvent<HelmStation, IgnoreKeypresses, &HelmStation::handleMockIgnore>})
    , ignoringMocks(false)
    , team(team_)
    , unit(unit_)
{}

HandleResult HelmStation::handleMockIgnore(IgnoreKeypresses* event)
{
    ignoringMocks = event->shouldIgnore;

    return HandleResult::Stop;
}
    

HandleResult HelmStation::handleKeypress(KeyEvent* keypress)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);
    if (ignoringMocks)
    {
        return HandleResult::Unhandled;
    }

    if (keypress->isDown == true)
    {
        ThrottleEvent event;
        event.team = team;
        event.unit = unit;

        switch (keypress->key)
        {
            case Key::RBracket:
            {
                // increase throttle!
                event.speed = lastState.speed + 1;

                EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));

                return HandleResult::Stop;
            }
            break;

            case Key::LBracket:
            {
                // decrease throttle
                event.speed = lastState.speed == 0 ? 0 : lastState.speed - 1;

                EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));
                return HandleResult::Stop;
            }
            break;

            default:
            break;
        }
    }

    // handle events where we want both keyup and keydown
    switch (keypress->key)
    {
        case Key::Left:
        {
            // Turn left
            SteeringEvent event;
            event.team = team;
            event.unit = unit;
            event.direction = SteeringEvent::Direction::Left;
            event.isPressed = keypress->isDown;
            EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));

            return HandleResult::Stop;
        }

        case Key::Right:
        {
            // Turn right
            SteeringEvent event;
            event.team = team;
            event.unit = unit;
            event.direction = SteeringEvent::Direction::Right;
            event.isPressed = keypress->isDown;
            EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));

            return HandleResult::Stop;
        }
        
        default:
        break;
    }

    return HandleResult::Unhandled;
}

HandleResult HelmStation::handleUnitState(UnitState* state)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);

    if (state->team == team && state->unit == unit) {
        Log::writeToLog(Log::L_DEBUG, "Got updated UnitState from server");
        lastState = *state;
        scheduleRedraw();
    }
    return HandleResult::Stop;
}

void HelmStation::redraw()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    Log::writeToLog(Log::L_DEBUG, "Renderer:", renderer, "Drawing updated speed:", lastState.speed);
    std::ostringstream sstream;
    sstream << "Speed:" << lastState.speed;
    drawText(sstream.str(), 20, 50, 50);
    //filledCircleColor(renderer, 100, 100, 50, 0xFFFFFFFF);

    SDL_RenderPresent(renderer);
}
