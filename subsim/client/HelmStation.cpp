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
        dispatchEvent<HelmStation, IgnoreKeypresses, &HelmStation::handleMockIgnore>,
        dispatchEvent<HelmStation, ScoreEvent, &HelmStation::handleScore>,
        })
    , ignoringMocks(false)
    , team(team_)
    , unit(unit_)
{}

HandleResult HelmStation::handleMockIgnore(IgnoreKeypresses* event)
{
    ignoringMocks = event->shouldIgnore;

    return HandleResult::Stop;
}

HandleResult HelmStation::handleScore(ScoreEvent* event)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);

    Log::writeToLog(Log::L_DEBUG, "Got updated score event!");

    scores = event->scores;
    return HandleResult::Continue;
}

HandleResult HelmStation::handleKeypress(KeyEvent* keypress)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);
    if (ignoringMocks)
    {
        return HandleResult::Unhandled;
    }

    if (keypress->isDown == false)
    {
        switch (keypress->key)
        {
            case Key::H:
            {
                PowerEvent event;
                event.team = team;
                event.unit = unit;
                event.system = PowerEvent::System::Yaw;
                event.isOn = !lastState.yawEnabled;

                EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));
                return HandleResult::Stop;
            }

            break;

            case Key::J:
            {
                PowerEvent event;
                event.team = team;
                event.unit = unit;
                event.system = PowerEvent::System::Engine;
                event.isOn = !lastState.engineEnabled;

                EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));
                return HandleResult::Stop;
            }
            break;

            case Key::K:
            {
                PowerEvent event;
                event.team = team;
                event.unit = unit;
                event.system = PowerEvent::System::Sonar;
                event.isOn = !lastState.sonarEnabled;

                EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));
                return HandleResult::Stop;
            }
            break;

            case Key::L:
            {
                PowerEvent event;
                event.team = team;
                event.unit = unit;
                event.system = PowerEvent::System::Weapons;
                event.isOn = !lastState.weaponsEnabled;

                EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));
                return HandleResult::Stop;
            }
            break;

            default:
            break;
        }
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
        lastState = *state;
        scheduleRedraw();
    }
    return HandleResult::Stop;
}

void HelmStation::redraw()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    std::ostringstream sstream;
    sstream << "Speed:" << lastState.speed;
    drawText(sstream.str(), 20, 50, 50);

    int x = 300;
    int y = 0;
    for (auto& scorePair : scores)
    {
        std::ostringstream ss;
        ss << "Team " << scorePair.first << ": " << scorePair.second;
        drawText(ss.str(), 20, x, y);
        x += 150;
    }

    if (lastState.yawEnabled)
    {
        filledCircleRGBA(renderer, 200, 50, 8, 0, 255, 0, 255);
    } else {
        filledCircleRGBA(renderer, 200, 50, 8, 255, 0, 0, 255);
    }
    drawText("Yaw steering", 20, 210, 36);
        
    if (lastState.engineEnabled)
    {
        filledCircleRGBA(renderer, 200, 70, 8, 0, 255, 0, 255);
    } else {
        filledCircleRGBA(renderer, 200, 70, 8, 255, 0, 0, 255);
    }
    drawText("Engine", 20, 210, 56);

    if (lastState.sonarEnabled)
    {
        filledCircleRGBA(renderer, 200, 90, 8, 0, 255, 0, 255);
    } else {
        filledCircleRGBA(renderer, 200, 90, 8, 255, 0, 0, 255);
    }
    drawText("Sonar", 20, 210, 76);

    if (lastState.weaponsEnabled)
    {
        filledCircleRGBA(renderer, 200, 110, 8, 0, 255, 0, 255);
    } else {
        filledCircleRGBA(renderer, 200, 110, 8, 255, 0, 0, 255);
    }
    drawText("Weapons", 20, 210, 96);

    SDL_RenderPresent(renderer);
}
