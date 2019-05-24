#include "HelmStation.h"

#include "../common/Messages.h"

#include "../common/Log.h"

#include "UI.h"
#include <SDL2_gfxPrimitives.h>

#include <sstream>

HelmStation::HelmStation(uint32_t team_, uint32_t unit_, std::map<uint32_t, std::string> teamNames_)
    : EventReceiver({
        dispatchEvent<HelmStation, KeyEvent, &HelmStation::handleKeypress>,
        dispatchEvent<HelmStation, UnitState, &HelmStation::handleUnitState>,
        dispatchEvent<HelmStation, IgnoreKeypresses, &HelmStation::handleMockIgnore>,
        dispatchEvent<HelmStation, ScoreEvent, &HelmStation::handleScore>,
        })
    , teamNames(teamNames_)
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
    scores = event->scores;
    return HandleResult::Continue;
}

HandleResult HelmStation::handleKeypress(KeyEvent* keypress)
{
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

    if (keypress->key == Key::Up)
    {
        ThrottleEvent event;
        event.team = team;
        event.unit = unit;
        if (keypress->isDown)
        {
            event.desiredSpeed = 1000;
        }
        else
        {
            event.desiredSpeed = 0;
        }
        EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));
        return HandleResult::Stop;
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
    if (state->team == team && state->unit == unit) {
        lastState = *state;
    }
    return HandleResult::Stop;
}

