#include "TacticalStation.h"

#include "../common/Messages.h"
#include "../common/Log.h"
#include "../common/ConfigParser.h"

#include "UI.h"
#include <SDL2_gfxPrimitives.h>
#include <sstream>

#define WIDTH 1280
#define HEIGHT 960

static constexpr uint32_t rgba_to_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | ((uint32_t)r);
}

TacticalStation::TacticalStation(uint32_t team_, uint32_t unit_, Config* config_)
    : Renderable(WIDTH, HEIGHT)
    , EventReceiver({
        dispatchEvent<TacticalStation, KeyEvent, &TacticalStation::handleKeypress>,
        dispatchEvent<TacticalStation, TextInputEvent, &TacticalStation::handleText>,
        dispatchEvent<TacticalStation, TextMessage, &TacticalStation::receiveTextMessage>,
        dispatchEvent<TacticalStation, UnitState, &TacticalStation::handleUnitState>,
        dispatchEvent<TacticalStation, SonarDisplayState, &TacticalStation::handleSonarDisplay>,
        dispatchEvent<TacticalStation, ExplosionEvent, &TacticalStation::handleExplosion>
    })
    , team(team_)
    , unit(unit_)
    , receivingText(false)
    , config(config_)
{}

TacticalStation::~TacticalStation()
{
    deregister();
}

HandleResult TacticalStation::handleKeypress(KeyEvent* keypress)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);

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

    if (keypress->isDown == false && keypress->key == Key::Backslash)
    {
        SonarEvent event;
        event.team = team;
        event.unit = unit;
        event.isActive = !lastState.isActiveSonar;

        EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(event));
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
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);

    Log::writeToLog(Log::L_DEBUG, "Received TextInput from the server. Text:", text->text);
    return HandleResult::Stop;
}

HandleResult TacticalStation::receiveTextMessage(TextMessage* message)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);

    Log::writeToLog(Log::L_DEBUG, "Received TextMessage from the server. Message:", message->message);

    return HandleResult::Stop;
}

HandleResult TacticalStation::handleUnitState(UnitState* state)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);

    if (state->team == team && state->unit == unit) {
        lastState = *state;
        scheduleRedraw();
    }
    return HandleResult::Stop;
}

HandleResult TacticalStation::handleSonarDisplay(SonarDisplayState* sonar)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);

    Log::writeToLog(Log::L_DEBUG, "Got updated SonarDisplay from server");
    lastSonar = *sonar;
    scheduleRedraw();
    return HandleResult::Stop;
}

HandleResult TacticalStation::handleExplosion(ExplosionEvent* explosion)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);
    explosions.push_back(*explosion);
    scheduleRedraw();
    return HandleResult::Stop;
}

void TacticalStation::redraw()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderSDTerrain();

    renderSDSubmarine(lastState.x, lastState.y, lastState.heading);

    for (const UnitSonarState &u : lastSonar.units)
    {
        if (u.team == team && u.unit == unit)
        {
            // Don't draw ourself this way
            continue;
        }
        renderSDSubmarine(u.x, u.y, u.heading);
    }

    for (const TorpedoState &torp : lastSonar.torpedos)
    {
        renderSDCircle(torp.x, torp.y, 10, rgba_to_color(255, 0, 0, 255));
    }

    for (const MineState &mine : lastSonar.mines)
    {
        renderSDCircle(mine.x, mine.y, 20, rgba_to_color(255, 0, 0, 255));
    }

    std::vector<ExplosionEvent> newExplosions;
    for (ExplosionEvent &exp : explosions)
    {
        renderSDCircle(exp.x, exp.y, exp.size, rgba_to_color(255, 0, 0, 255));
        exp.size -= 5;
        if (exp.size > 0) {
            newExplosions.push_back(exp);
        }
    }
    explosions = std::move(newExplosions);

    renderTubeState();
    renderSonarState();

    SDL_RenderPresent(renderer);
}

void TacticalStation::renderSonarState()
{
    boxRGBA(renderer, 120, 0, 240, 20, 0, 0, 0, 255);
    if (lastState.isActiveSonar)
    {
        drawText("Active sonar", 18, 125, 0);
    } else {
        drawText("Passive sonar", 18, 125, 0);
    }
}
    

void TacticalStation::renderTubeState()
{
    // Use green for open tubes, red for armed tubes
    uint32_t openColor = rgba_to_color(0, 255, 0, 255);
    uint32_t armedColor = rgba_to_color(255, 0, 0, 255);
    
    std::ostringstream t_sstream, m_sstream;
    t_sstream << "Torpedos:" << lastState.remainingTorpedos;
    m_sstream << "Mines:" << lastState.remainingMines;

    boxRGBA(renderer, 0, 0, 120, 60, 0, 0, 0, 255);

    drawText(t_sstream.str(), 18, 5, 0);
    drawText(m_sstream.str(), 18, 5, 17);


    for (int i = 0; i < lastState.tubeOccupancy.size(); ++i)
    {
        uint32_t color = lastState.tubeIsArmed[i] ? armedColor : openColor;

        int x = 10 + 25 * i;
        int y = 50;

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

void TacticalStation::renderSDTerrain()
{
    int32_t s = config->terrain.scale; //scale of map
    int32_t tx_min = (lastState.x - 2*config->sonarRange) / s;
    int32_t tx_max = (lastState.x + 2*config->sonarRange) / s;
    int32_t ty_min = (lastState.y - 2*config->sonarRange) / s;
    int32_t ty_max = (lastState.y + 2*config->sonarRange) / s;
    uint32_t terrainColor = rgba_to_color(100, 100, 100, 255);

    for (int32_t tx = tx_min; tx <= tx_max; ++tx)
    {
        for (int32_t ty = ty_min; ty <= ty_max; ++ty)
        {
            if (config->terrain.colorAt(tx, ty) == Terrain::WALL)
            {
                int64_t xs[4] = {tx*s, (tx+1)*s, (tx+1)*s, tx*s};
                int64_t ys[4] = {ty*s, ty*s, (ty+1)*s, (ty+1)*s};
                renderSDFilledPolygon(xs, ys, 4, terrainColor);
            }
        }
    }

    uint32_t gridColor = rgba_to_color(100, 100, 100, 255);
    for (int64_t tx = tx_min; tx <= tx_max; ++tx)
    {
        renderSDLine(tx*s, ty_min*s, tx*s, ty_max*s, gridColor);
    }
    for (int64_t ty = ty_min; ty < ty_max; ++ty)
    {
        renderSDLine(ty_min*s, ty*s, ty_max*s, ty*s, gridColor);
    }
}

void TacticalStation::renderSDSubmarine(int64_t x, int64_t y, int16_t heading)
{
    float u = cos(heading * 2*M_PI/360.0);
    float v = sin(heading * 2*M_PI/360.0);
    uint32_t color = rgba_to_color(255, 0, 0, 255);
    renderSDArc(x+u*10, y+v*10, 10, heading+90, heading-90, color);
    renderSDArc(x-u*10, y-v*10, 10, heading-90, heading+90, color);
    renderSDLine(x+u*10+v*10, y+v*10-u*10, x-u*10+v*10, y-v*10-u*10, color);
    renderSDLine(x+u*10-v*10, y+v*10+u*10, x-u*10-v*10, y-v*10+u*10, color);
}

void TacticalStation::renderSDCircle(int64_t x, int64_t y, int16_t r, uint32_t color)
{
    circleColor(renderer, sdX(x, y), sdY(x, y), sdRadius(r), color);
}

void TacticalStation::renderSDLine(int64_t x1, int64_t y1, int64_t x2, int64_t y2, uint32_t color)
{
    lineColor(renderer, sdX(x1, y1), sdY(x1, y1), sdX(x2, y2), sdY(x2, y2), color);
}

void TacticalStation::renderSDArc(int64_t x, int64_t y, int16_t r, int16_t a1, int16_t a2, uint32_t color)
{
    arcColor(renderer, sdX(x, y), sdY(x, y), sdRadius(r), sdHeading(a1), sdHeading(a2), color);
}

void TacticalStation::renderSDFilledPolygon(const int64_t *xs, const int64_t *ys, int count, uint32_t color) {
    std::vector<int16_t> transformed_xs(count);
    std::vector<int16_t> transformed_ys(count);
    for (int i = 0; i < count; ++i) {
      transformed_xs[i] = sdX(xs[i], ys[i]);
      transformed_ys[i] = sdY(xs[i], ys[i]);
    }
    filledPolygonColor(renderer, transformed_xs.data(), transformed_ys.data(), count, color);
}

int64_t TacticalStation::sdX(int64_t x, int64_t y)
{
    float xx = (x - lastState.x) * sin(lastState.heading * 2*M_PI/360.0)
             - (y - lastState.y) * cos(lastState.heading * 2*M_PI/360.0);
    xx = xx / config->sonarRange * (WIDTH/2);
    return WIDTH/2 + xx;
}

int64_t TacticalStation::sdY(int64_t x, int64_t y)
{
    float yy = - (x - lastState.x) * cos(lastState.heading * 2*M_PI/360.0)
               - (y - lastState.y) * sin(lastState.heading * 2*M_PI/360.0);
    yy = yy / config->sonarRange * (WIDTH/2);
    return HEIGHT/2 + yy;
}

int16_t TacticalStation::sdRadius(int16_t r)
{
    return float(r) / config->sonarRange * (WIDTH/2);
}

int16_t TacticalStation::sdHeading(int16_t heading)
{
    return 270 - (heading - lastState.heading);
}


