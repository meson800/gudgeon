#include "UI.h"

#include "../common/Log.h"
#include "../common/Exceptions.h"
#include "MockUIEvents.h"

#include "SDL.h"
#include "SDL_ttf.h"




/// Static definition of singleton
UI* UI::singleton = nullptr;

UI::UI()
    : shouldShutdown(false)
    , shouldUpdateText(false)
    , textStatus(false)
{
    std::mutex startupMux;
    bool startupDone = false;

    Log::writeToLog(Log::INFO, "Starting SDL thread...");
    // Start thread, passing in the startup references
    sdlThread = std::thread(&UI::runSDLloop, this, std::ref(startupDone), std::ref(startupMux));

    // Busy wait until startup complete
    while (true)
    {
        std::lock_guard<std::mutex> guard(startupMux);
        if (startupDone)
        {
            Log::writeToLog(Log::INFO, "SDL thread startup complete");
            return;
        }
    }
}

UI::~UI()
{
    Log::writeToLog(Log::INFO, "Shutting down SDL...");
    {
        std::lock_guard<std::mutex> lock(shutdownMux);
        shouldShutdown = true;
    }
    sdlThread.join();
    Log::writeToLog(Log::INFO, "SDL thread shutdown complete");
}

UI* UI::getGlobalUI()
{
    if (singleton == nullptr)
    {
        Log::writeToLog(Log::ERR, "Attempted to use a singleton UI class before assigning one!");
        throw std::runtime_error("Attempted to use unset UI singleton!");
    }
    return singleton;
}

void UI::setGlobalUI(UI* singleton_)
{
    if (singleton != nullptr)
    {
        Log::writeToLog(Log::ERR, "Attempted to assign a singleton UI class when one already existed!");
        throw std::runtime_error("Attempted to set singleton twice!");
    }
    singleton = singleton_;
}

SDL_Renderer* UI::getFreeRenderer(uint16_t minWidth, uint16_t minHeight)
{
    // At the moment, just allocate a new window and renderer for every call
    SDL_Window * newWindow;
    SDL_Renderer * newRenderer;
    
    if (SDL_CreateWindowAndRenderer(minWidth, minHeight, 0, &newWindow, &newRenderer) != 0)
    {
        Log::writeToLog(Log::ERR, "Couldn't create a new window and renderer of size (", minWidth, ",", minHeight, ")");
        throw SDLError("Error in SDL_CreateWindowAndRenderer");
    }

    // Clear the screen initially
    SDL_SetRenderDrawColor(newRenderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(newRenderer);
    SDL_RenderPresent(newRenderer);

    windows.push_back(newWindow);
    renderers.push_back(newRenderer);
    ownedWindows[newRenderer] = newWindow;

    return newRenderer;
}

Renderable::Renderable(SDL_Renderer* renderer_)
{
    Log::writeToLog(Log::L_DEBUG, "Attaching renderable ", this, " to pre-existing renderer ", renderer_);
    UI::getGlobalUI()->registerRenderable(renderer, this);
    renderer = renderer_;
}

Renderable::Renderable(uint16_t width, uint16_t height)
    : renderer(nullptr)
{
    Log::writeToLog(Log::L_DEBUG, "Requesting a new renderer with dimensions ", width, " x ", height);
    UI::getGlobalUI()->requestRenderer(width, height, &renderer, this);
}

Renderable::~Renderable()
{
    Log::writeToLog(Log::L_DEBUG, "Renderable deconstructor called for ", this);
    if (renderer != nullptr)
    {
        UI::getGlobalUI()->deregisterRenderable(renderer, this);
    }

    // Cleanup the font and texture caches
    for (auto font_pair : fontCache)
    {
        TTF_CloseFont(font_pair.second);
    }

    for (auto& texts : textCache)
    {
        for (auto tex_pair : texts.second)
        {
            SDL_DestroyTexture(tex_pair.second);
        }
    }
}

void Renderable::scheduleRedraw()
{
    Log::writeToLog(Log::L_DEBUG, "Redraw requested from renderable ", this);
    if (renderer != nullptr)
    {
        UI::getGlobalUI()->triggerRedraw(renderer);
    }
}

void Renderable::drawText(const std::string& text, uint8_t fontsize, uint16_t x, uint16_t y, bool shouldCache)
{
    if (fontCache.count(fontsize) == 0)
    {
        // Open the font in this font size
        TTF_Font* font = TTF_OpenFont("data/sans.ttf", fontsize);
        if (!font)
        {
            Log::writeToLog(Log::ERR, "Failed to open font! TTF error:", TTF_GetError());
            throw SDLError("Failed to open font");
        }
            
        fontCache[fontsize] = font;

        // Create a new text cache
        textCache[fontsize] = std::map<std::string, SDL_Texture*>{};
    }

    SDL_Texture* renderedText;
    if (shouldCache == false || textCache[fontsize].count(text) == 0)
    {
        // Render new fonts
        SDL_Color white = {255, 255, 255};
        // Render in quick-in-dirty mode (Solid) if we are not caching, otherwise render blended
        SDL_Surface* surfaceVersion = shouldCache ? 
            TTF_RenderText_Blended(fontCache[fontsize], text.c_str(), white)
            : TTF_RenderText_Solid(fontCache[fontsize], text.c_str(), white);

        if (!surfaceVersion)
        {
            Log::writeToLog(Log::ERR, "Failed to render text at font size ", fontsize, " and text='", text.c_str(), "'. TTF error:", TTF_GetError());
            throw SDLError("Failed to render text");
        }

        renderedText = SDL_CreateTextureFromSurface(renderer, surfaceVersion);

        if (!renderedText)
        {
            Log::writeToLog(Log::ERR, "Unable to create texture from rendered surface. SDL error:", SDL_GetError());
            throw SDLError("Failed to convert rendered text to surface");
        }

        if (shouldCache)
        {
            textCache[fontsize][text] = renderedText;
        }
        SDL_FreeSurface(surfaceVersion);
    } else {
        // lookup from cache
        renderedText = textCache[fontsize][text];
    }

    int width, height;
    TTF_SizeText(fontCache[fontsize], text.c_str(), &width, &height);

    SDL_Rect messageRect;
    messageRect.x = x;
    messageRect.y = y;
    messageRect.w = width;
    messageRect.h = height;

    SDL_RenderCopy(renderer, renderedText, NULL, &messageRect);

    if (shouldCache == false)
    {
        SDL_DestroyTexture(renderedText);
    }
}

void UI::requestRenderer(uint16_t width, uint16_t height, SDL_Renderer** renderer, Renderable* target)
{
    Log::writeToLog(Log::L_DEBUG, "Recieved new renderer request from renderable ", target);
    RendererRequest request;
    request.width = width;
    request.height = height;
    request.renderer = renderer;
    request.target = target;
    {
        std::lock_guard<std::mutex> lock(stateMux);
        rendererRequests.push_back(request);
    }
}

void UI::registerRenderable(SDL_Renderer* renderer, Renderable* renderable)
{
    std::lock_guard<std::mutex> lock(stateMux);
    internalRegisterRenderable(renderer, renderable);
}

void UI::internalRegisterRenderable(SDL_Renderer* renderer, Renderable* renderable)
{
    if (renderStack[renderer].insert(renderable).second == false)
    {
        Log::writeToLog(Log::WARN, "Renderable callback class ", renderable, "already registered on renderer ", renderer, "! Ignoring.");
    } else {
        Log::writeToLog(Log::L_DEBUG, "Registered renderable ", renderable, " on renderer ", renderer);
        // Schedule a redraw as we've added a new item to the render stack
        triggerRedraw(renderer);
    }
}

void UI::deregisterRenderable(SDL_Renderer* renderer, Renderable* renderable)
{
    std::lock_guard<std::mutex> lock(stateMux);
    auto it = renderStack[renderer].find(renderable);

    if (it == renderStack[renderer].end())
    {
        Log::writeToLog(Log::ERR, "Attempted to remove rendererable ", renderable, " from renderer ", renderer,
            " that was not registered!");
        throw std::runtime_error("Removal of unregistered renderable attempted!");
    }

    Log::writeToLog(Log::L_DEBUG, "Deregistering renderable ", renderable);

    renderStack[renderer].erase(renderable);

    // Set this renderer to destroy
    toDestroy.push_back(renderable);

    Log::writeToLog(Log::L_DEBUG, "Deregistered renderable ", renderable, " from renderer ", renderer);
}

void UI::triggerRedraw(SDL_Renderer* renderer)
{
    Log::writeToLog(Log::L_DEBUG, "Scheduling redraw for renderer ", renderer);
    std::lock_guard<std::mutex> guard(redrawRequestMux);
    toRedraw.insert(renderer);
}

void UI::runSDLloop(bool& startupDone, std::mutex& startupMux)
{
    {
        // Lock the startup section in a lock-guard, so we release even if 
        // an exception is thrown
        std::lock_guard<std::mutex> lock(startupMux);

        if (TTF_Init() == -1)
        {
            Log::writeToLog(Log::ERR, "Couldn't start SDL TTF library! TTF error:", TTF_GetError());
            throw SDLError("Error in TTF_Init");
        }

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        {
            Log::writeToLog(Log::ERR, "Couldn't start SDL! SDL error:", SDL_GetError());
            throw SDLError("Error in SDL_Init");
        }
        SDL_StopTextInput();
        startupDone = true;
    }

    // Now run the main redraw loop, checking if we should exit or not
    while (true)
    {
        /* Check for shutdown inside a locked mutex */
        {
            std::lock_guard<std::mutex> lock(shutdownMux);

            if (shouldShutdown)
            {
                // Cleanup the renderers, then cleanup the windows
                for (SDL_Renderer* renderer : renderers)
                {
                    SDL_DestroyRenderer(renderer);
                }

                for (SDL_Window* window : windows)
                {
                    SDL_DestroyWindow(window);
                }
                TTF_Quit();
                SDL_Quit();
                return;
            }
        }
        // Lock the state mutex and fulfill any window requests
        {
            std::lock_guard<std::mutex> lock(stateMux);

            for (auto request : rendererRequests)
            {
                *(request.renderer) = getFreeRenderer(request.width, request.height);
                internalRegisterRenderable(*(request.renderer), request.target);
                Log::writeToLog(Log::L_DEBUG, "Fulfilled new renderer request for renderable ", request.target);
            }
            rendererRequests.clear();

            // Now, iterate over the renderers and see if we need to destroy anything
            if (toDestroy.size() > 0)
            {
                toDestroy.clear();

                auto it = renderStack.begin();
                while (it != renderStack.end())
                {
                    if (it->second.size() == 0)
                    {
                        //This renderable is empty. Destroy it
                        SDL_DestroyRenderer(it->first);
                        SDL_DestroyWindow(ownedWindows[it->first]);
                        ownedWindows.erase(it->first);
                        it = renderStack.erase(it);
                    } else {
                        ++it;
                    }
                }
            }


        }
        // Update text status if required
        if (shouldUpdateText)
        {
            if (textStatus)
            {
                Log::writeToLog(Log::L_DEBUG, "Enabling SDL text input");
                SDL_StartTextInput();
            } else {
                Log::writeToLog(Log::L_DEBUG, "Disabling SDL text input");
                SDL_StopTextInput();
            }
            shouldUpdateText = false;
        }

        // Get SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                {
                    KeyEvent ke;
                    ke.isDown = event.type == SDL_KEYDOWN;
                    ke.repeat = event.key.repeat;

                    switch (event.key.keysym.sym)
                    {
                        case SDLK_LEFT:
                        ke.key = Key::Left;
                        break;

                        case SDLK_RIGHT:
                        ke.key = Key::Right;
                        break;

                        case SDLK_UP:
                        ke.key = Key::Up;
                        break;

                        case SDLK_DOWN:
                        ke.key = Key::Down;
                        break;

                        case SDLK_RETURN:
                        ke.key = Key::Enter;
                        break;

                        case SDLK_LEFTBRACKET:
                        ke.key = Key::LBracket;
                        break;

                        case SDLK_RIGHTBRACKET:
                        ke.key = Key::RBracket;
                        break;

                        case SDLK_SPACE:
                        ke.key = Key::Space;
                        break;

                        case SDLK_1:
                        ke.key = Key::One;
                        break;

                        case SDLK_2:
                        ke.key = Key::Two;
                        break;

                        case SDLK_3:
                        ke.key = Key::Three;
                        break;

                        case SDLK_4:
                        ke.key = Key::Four;
                        break;

                        case SDLK_5:
                        ke.key = Key::Five;
                        break;

                        case SDLK_q:
                        ke.key = Key::Q;
                        break;

                        case SDLK_w:
                        ke.key = Key::W;
                        break;

                        case SDLK_e:
                        ke.key = Key::E;
                        break;

                        case SDLK_r:
                        ke.key = Key::R;
                        break;

                        case SDLK_t:
                        ke.key = Key::T;
                        break;

                        case SDLK_a:
                        ke.key = Key::A;
                        break;

                        case SDLK_s:
                        ke.key = Key::S;
                        break;

                        case SDLK_d:
                        ke.key = Key::D;
                        break;

                        case SDLK_f:
                        ke.key = Key::F;
                        break;

                        case SDLK_g:
                        ke.key = Key::G;
                        break;

                        case SDLK_h:
                        ke.key = Key::H;
                        break;

                        case SDLK_j:
                        ke.key = Key::J;
                        break;

                        case SDLK_k:
                        ke.key = Key::K;
                        break;

                        case SDLK_l:
                        ke.key = Key::L;
                        break;

                        default:
                        ke.key = Key::Other;
                    }

                    EventSystem::getGlobalInstance()->queueEvent(ke);

                }
                break;

                case SDL_TEXTINPUT:
                {
                    TextInputEvent te;
                    te.text = event.text.text;
                    EventSystem::getGlobalInstance()->queueEvent(te);
                }
                break;

                default:
                break;
            }
        }


        // Lock the redraw mutex and redraw any renders that have requested it
        {
            std::lock_guard<std::mutex> lock(redrawMux);
            // lock the state mux as well, in case a client tries to deregister in the middle
            std::lock_guard<std::mutex> lock2(stateMux);

            std::set<SDL_Renderer*> redrawSet;

            {
                std::lock_guard<std::mutex> requestLock(redrawRequestMux);
                redrawSet = toRedraw;
                toRedraw.clear();
            }

            for (SDL_Renderer* renderer : redrawSet)
            {

                if (renderStack.count(renderer) == 0)
                {
                    Log::writeToLog(Log::WARN, "Redraw requested for nonexistent renderer ", renderer, "!");
                    continue;
                }
                Log::writeToLog(Log::L_DEBUG, "Redrawing renderer ", renderer);

                for (Renderable* renderable : renderStack[renderer])
                {
                    renderable->redraw();
                }
            }
        }
                

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    }
}

void UI::changeTextInput(bool receiveText)
{
   textStatus =  receiveText;
   shouldUpdateText = true;
}

