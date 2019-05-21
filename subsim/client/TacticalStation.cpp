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
        return HandleResult::Stop;
    }

    if (keypress->isDown == false && keypress->key == Key::Space)
    {
        FireEvent fire;
        fire.team = team;
        fire.unit = unit;

        EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(fire));
        Log::writeToLog(Log::L_DEBUG, "Fired torpedos/mines");
        return HandleResult::Stop;
    }

    // Perform tube mocks
    if (keypress->isDown == false)
    {
        TubeLoadEvent tubeLoad;
        TubeArmEvent tubeArm;
        tubeLoad.team = team;
        tubeLoad.unit = unit;
        tubeArm.team = team;
        tubeArm.unit = unit;

        bool loaded = false;
        bool armed = false;
        switch (keypress->key)
        {
            case Key::One:
                tubeArm.tube = 0;
                armed = true;
            break;

            case Key::Two:
                tubeArm.tube = 1;
                armed = true;
            break;

            case Key::Three:
                tubeArm.tube = 2;
                armed = true;
            break;

            case Key::Four:
                tubeArm.tube = 3;
                armed = true;
            break;

            case Key::Five:
                tubeArm.tube = 4;
                armed = true;
            break;

            case Key::Q:
                tubeLoad.tube = 0;
                tubeLoad.type = TubeLoadEvent::AmmoType::Torpedo;
                loaded = true;
            break;

            case Key::W:
                tubeLoad.tube = 1;
                tubeLoad.type = TubeLoadEvent::AmmoType::Torpedo;
                loaded = true;
            break;

            case Key::E:
                tubeLoad.tube = 2;
                tubeLoad.type = TubeLoadEvent::AmmoType::Torpedo;
                loaded = true;
            break;

            case Key::R:
                tubeLoad.tube = 3;
                tubeLoad.type = TubeLoadEvent::AmmoType::Torpedo;
                loaded = true;
            break;

            case Key::T:
                tubeLoad.tube = 4;
                tubeLoad.type = TubeLoadEvent::AmmoType::Torpedo;
                loaded = true;
            break;

            case Key::A:
                tubeLoad.tube = 0;
                tubeLoad.type = TubeLoadEvent::AmmoType::Mine;
                loaded = true;
            break;

            case Key::S:
                tubeLoad.tube = 1;
                tubeLoad.type = TubeLoadEvent::AmmoType::Mine;
                loaded = true;
            break;

            case Key::D:
                tubeLoad.tube = 2;
                tubeLoad.type = TubeLoadEvent::AmmoType::Mine;
                loaded = true;
            break;

            case Key::F:
                tubeLoad.tube = 3;
                tubeLoad.type = TubeLoadEvent::AmmoType::Mine;
                loaded = true;
            break;

            case Key::G:
                tubeLoad.tube = 4;
                tubeLoad.type = TubeLoadEvent::AmmoType::Mine;
                loaded = true;
            break;

            default:
            break;
        }
        if (armed)
        {
            // we armed one of the tubes, send mock along after updating state
            tubeArm.isArmed = !lastState.tubeIsArmed[tubeArm.tube];
            EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(tubeArm));
            return HandleResult::Stop;
        }

        if (loaded)
        {
            EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(tubeLoad));
            return HandleResult::Stop;
        }
    }

    return HandleResult::Unhandled;
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

    renderTubeState();

    SDL_RenderPresent(renderer);
}

static constexpr uint32_t rgba_to_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | ((uint32_t)r);
}

void TacticalStation::renderTubeState()
{
    // Use green for open tubes, red for armed tubes
    uint32_t openColor = rgba_to_color(0, 255, 0, 255);
    uint32_t armedColor = rgba_to_color(255, 0, 0, 255);
    
    for (int i = 0; i < lastState.tubeOccupancy.size(); ++i)
    {
        uint32_t color = lastState.tubeIsArmed[i] ? armedColor : openColor;

        int x = 10 + 25 * i;
        int y = 10;

        switch(lastState.tubeOccupancy[i])
        {
            case UnitState::TubeStatus::Empty:
                filledCircleColor(renderer, x, y, 8, color);
            break;

            case UnitState::TubeStatus::Torpedo:
                boxColor(renderer, x - 6, y - 5, x + 6, y + 7, color);
                filledPieColor(renderer, x, y - 5, 6, 180, 360, color);
            break;

            case UnitState::TubeStatus::Mine:
                filledCircleColor(renderer, x, y, 5, color);
                // draw spokes, because this is a mine
                for (int i = 0; i < 360; i += 60)
                {
                    thickLineColor(renderer, x, y
                        , x + cos(i * 2 * M_PI / 360.0) * 10
                        , y + sin(i * 2 * M_PI / 360.0) * 10
                        , 1, color);
                }
            break;

            default:
            break;
        }
    }
        
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


