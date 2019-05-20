#include "HelmStation.h"

#include "../common/Messages.h"

#include "../common/Log.h"

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
    if (ignoringMocks)
    {
        return HandleResult::Unhandled;
    }

    if (keypress->isDown == true)
    {
        ThrottleEvent event;
        event.team = team;
        event.unit = unit;
        if (keypress->key == Key::LBracket)
        {
            // increase throttle!
            event.speed = lastState.speed + 1;

            EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));

            return HandleResult::Stop;
        } else if (keypress->key == Key::RBracket) {
            // decrease throttle
            event.speed = lastState.speed == 0 ? 0 : lastState.speed - 1;

            EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));
            return HandleResult::Stop;
        }
    }
    return HandleResult::Unhandled;
}

HandleResult HelmStation::handleUnitState(UnitState* state)
{
    lastState = *state;
    scheduleRedraw();
    return HandleResult::Stop;
}

void HelmStation::redraw()
{
    std::ostringstream sstream;
    sstream << "Speed:" << lastState.speed;
    drawText(sstream.str(), 20, 50, 50);
}
