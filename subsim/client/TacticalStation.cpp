#include "TacticalStation.h"

#include "../common/Messages.h"
#include "../common/Log.h"
#include "../common/ConfigParser.h"
#include "../common/Exceptions.h"

#include "UI.h"
#include <SDL2_gfxPrimitives.h>
#include <sstream>

#define WIDTH 1280
#define HEIGHT 960

static constexpr uint32_t rgba_to_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | ((uint32_t)r);
}

constexpr uint32_t friendColor = rgba_to_color(0, 255, 0, 255);
constexpr uint32_t stealthColor = rgba_to_color(0, 70, 0, 255);
constexpr uint32_t enemyColor = rgba_to_color(255, 0, 0, 255);

TacticalStation::TacticalStation(uint32_t team_, uint32_t unit_, Config* config_)
    : Renderable(WIDTH, HEIGHT)
    , EventReceiver({
        dispatchEvent<TacticalStation, KeyEvent, &TacticalStation::handleKeypress>,
        dispatchEvent<TacticalStation, TextInputEvent, &TacticalStation::handleText>,
        dispatchEvent<TacticalStation, TextMessage, &TacticalStation::receiveTextMessage>,
        dispatchEvent<TacticalStation, UnitState, &TacticalStation::handleUnitState>,
        dispatchEvent<TacticalStation, SonarDisplayState, &TacticalStation::handleSonarDisplay>,
        dispatchEvent<TacticalStation, ExplosionEvent, &TacticalStation::handleExplosion>,
        dispatchEvent<TacticalStation, ScoreEvent, &TacticalStation::handleScores>,
    })
    , team(team_)
    , unit(unit_)
    , receivingText(false)
    , config(config_)
    , terrainTexture(NULL)
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
        StealthEvent event;
        event.team = team;
        event.unit = unit;
        event.isStealth = !lastState.isStealth;

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

    lastSonar = *sonar;
    scheduleRedraw();
    return HandleResult::Stop;
}

HandleResult TacticalStation::handleScores(ScoreEvent* event)
{
    std::lock_guard<std::mutex> lock(UI::getGlobalUI()->redrawMux);
    scores = event->scores;
    return HandleResult::Continue;
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
    initializeRendering();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderSDTerrain();

    uint32_t ownColor;
    if (!lastState.isStealth)
    {
        ownColor = friendColor;
    } else if (lastState.isStealth && lastState.stealthCooldown == 0) {
        ownColor = stealthColor;
    } else {
        // linearlly interpolate color if in cooldown
        uint8_t lerp = 70 + (255 - 70) 
            * ((double)lastState.stealthCooldown / (double)config->stealthCooldown);

        ownColor = rgba_to_color(0, lerp, 0, 255);
    }
    renderSDSubmarine(
        lastState.x, lastState.y, lastState.heading,
        lastState.hasFlag,
        ownColor, enemyColor);

    for (const UnitSonarState &u : lastSonar.units)
    {
        if (u.team == team && u.unit == unit)
        {
            // Don't draw ourself this way
            continue;
        }
        uint8_t colorIntensity;
        if (!u.isStealth)
        {
            colorIntensity = 255;
        } else if (u.isStealth && u.stealthCooldown > 0) {
            colorIntensity = 255 * (double) u.stealthCooldown / (double) config->stealthCooldown;
        } else {
            colorIntensity = 0;
        }
        renderSDSubmarine(
            u.x, u.y, u.heading,
            u.hasFlag,
            rgba_to_color(colorIntensity, colorIntensity, colorIntensity, 255),
            // If the sub is on our team, assume it's carrying an enemy flag,
            // and vice versa
            (u.team == team) ? enemyColor : friendColor);

        // Draw targeting reticule
        if (lastState.targetIsLocked &&
            u.team == lastState.targetTeam &&
            u.unit == lastState.targetUnit)
        {
            int16_t x = sdX(u.x, u.y), y = sdY(u.x, u.y);
            uint32_t color = rgba_to_color(0, 0, 255, 255);
            circleColor(renderer, x, y, 30, color);
            lineColor(renderer, x+20, y, x+35, y, color);
            lineColor(renderer, x, y+20, x, y+35, color);
            lineColor(renderer, x-20, y, x-35, y, color);
            lineColor(renderer, x, y-20, x, y-35, color);
        }
    }

    for (const TorpedoState &torp : lastSonar.torpedos)
    {
        float u = cos(torp.heading * 2*M_PI/360.0);
        float v = sin(torp.heading * 2*M_PI/360.0);
        uint32_t color = rgba_to_color(255, 255, 255, 255);
        renderSDLine(torp.x-u*50, torp.y-v*50, torp.x+u*50, torp.y+v*50, color);
    }

    for (const MineState &mine : lastSonar.mines)
    {
        renderSDCircle(mine.x, mine.y, 200, rgba_to_color(255, 255, 255, 255));
    }

    std::vector<ExplosionEvent> newExplosions;
    for (ExplosionEvent &exp : explosions)
    {
        renderSDCircle(exp.x, exp.y, exp.size*10, rgba_to_color(200, 200, 200, 255));
        exp.size -= 2;
        if (exp.size > 0) {
            newExplosions.push_back(exp);
        }
    }
    explosions = std::move(newExplosions);

    uint32_t mineExclusionColor = rgba_to_color(255, 255, 255, 255);

    for (const FlagState &flag : lastSonar.flags)
    {
        if (!flag.isTaken)
        {
            uint32_t color = flag.team == team ? friendColor : enemyColor;
            renderSDFlag(flag.x, flag.y, color);
        }

        // render exclusion zone even if flag is taken
        renderSDCircle(flag.x, flag.y, config->mineExclusionRadius, mineExclusionColor);
    }

    // render starting locations
    for (const auto &startPair : config->startLocations)
    {
        auto startLoc = startPair.second.at(0);
        uint32_t color = startPair.first == team ? friendColor : enemyColor;
        renderSDCircle(startLoc.first, startLoc.second, 200, color);
        renderSDCircle(startLoc.first, startLoc.second, config->mineExclusionRadius, mineExclusionColor);
    }

    renderTubeState();
    renderStealthState();

    int x = 300;
    int y = 0;
    for (auto& scorePair : scores)
    {
        std::ostringstream ss;
        ss << "Team " << scorePair.first << ": " << scorePair.second;
        drawText(ss.str(), 20, x, y);
        x += 150;
    }

    SDL_RenderPresent(renderer);
}

void TacticalStation::initializeRendering() {
    /* This code should only run once */
    if (terrainTexture != NULL) {
        return;
    }

    /* Dump rendering info to the log file */
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer, &info) != 0) {
        throw SDLError("Error in SDL_GetRendererInfo");
    }
    Log::writeToLog(Log::L_DEBUG, "Renderer name ", info.name);
    Log::writeToLog(Log::L_DEBUG, "SDL_RENDERER_SOFTWARE? ", info.flags & SDL_RENDERER_SOFTWARE);
    Log::writeToLog(Log::L_DEBUG, "SDL_RENDERER_ACCELERATED? ", info.flags & SDL_RENDERER_ACCELERATED);
    Log::writeToLog(Log::L_DEBUG, "SDL_RENDERER_PRESENTVSYNC? ", info.flags & SDL_RENDERER_PRESENTVSYNC);
    Log::writeToLog(Log::L_DEBUG, "SDL_RENDERER_TARGETTEXTURE? ", info.flags & SDL_RENDERER_TARGETTEXTURE);

    /* Initialize terrainTexture */
    terrainTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STATIC,
        1, // width
        1  // height
    );
    if (!terrainTexture)
    {
        throw SDLError("SDL_UpdateTexture");
    }
    uint8_t pixel[4] = {100, 100, 100, 255};
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = 1;
    rect.h = 1;
    if (SDL_UpdateTexture(terrainTexture, &rect, &pixel, 4) != 0)
    {
        throw SDLError("SDL_UpdateTexture");
    }
}

void TacticalStation::renderStealthState()
{
    boxRGBA(renderer, 120, 0, 290, 20, 0, 0, 0, 255);
    if (!lastState.isStealth)
    {
        drawText("Active mode", 18, 125, 0);
    } else {
        if (lastState.stealthCooldown > 0)
        {
            drawText("Stealth activating...", 18, 125, 0);
        } 
        else
        {
            drawText("Stealth mode", 18, 125, 0);
        }
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

    for (int32_t tx = tx_min; tx <= tx_max; ++tx)
    {
        for (int32_t ty = ty_min; ty <= ty_max; ++ty)
        {
            if (config->terrain.colorAt(tx, ty) == Terrain::WALL)
            {
                int16_t centerX = sdX(tx*s + s/2, ty*s + s/2);
                int16_t centerY = sdY(tx*s + s/2, ty*s + s/2);
                SDL_Rect dstRect;
                dstRect.w = dstRect.h = sdRadius(s) + 3;
                dstRect.x = centerX - dstRect.w / 2;
                dstRect.y = centerY - dstRect.h / 2;
                SDL_RenderCopyEx(
                    renderer,
                    terrainTexture,
                    NULL,
                    &dstRect,
                    sdHeading(0),
                    NULL,
                    SDL_FLIP_NONE);
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
        renderSDLine(tx_min*s, ty*s, tx_max*s, ty*s, gridColor);
    }
}

void TacticalStation::renderSDSubmarine(
    int64_t x, int64_t y, int16_t heading,
    bool hasFlag,
    uint32_t color, uint32_t flagColor)
{
    float u = cos(heading * 2*M_PI/360.0);
    float v = sin(heading * 2*M_PI/360.0);

    renderSDArc(x+u*100, y+v*100, 100, heading+90, heading-90, color);
    renderSDArc(x-u*100, y-v*100, 100, heading-90, heading+90, color);
    renderSDLine(x+u*100+v*100, y+v*100-u*100, x-u*100+v*100, y-v*100-u*100, color);
    renderSDLine(x+u*100-v*100, y+v*100+u*100, x-u*100-v*100, y-v*100+u*100, color);
    renderSDCircle(x+u*70, y+v*70, 40, color);

    if (hasFlag)
    {
        renderSDFlag(x, y, flagColor);
    }
}

void TacticalStation::renderSDFlag(int64_t x, int64_t y, uint32_t color)
{
    int16_t newX = sdX(x, y);
    int16_t newY = sdY(x, y);

    int16_t x_locs [6];
    x_locs[0] = newX;
    x_locs[1] = newX;
    x_locs[2] = newX + 30;
    x_locs[3] = newX + 30;
    x_locs[4] = newX + 4;
    x_locs[5] = newX + 4;

    int16_t y_locs [6];
    y_locs[0] = newY;
    y_locs[1] = newY - 30;
    y_locs[2] = newY - 30;
    y_locs[3] = newY - 13;
    y_locs[4] = newY - 13;
    y_locs[5] = newY;
    filledPolygonColor(renderer, x_locs, y_locs, 6, color);
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


