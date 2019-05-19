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
    SDL_SetRenderDrawColor(newRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
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
    std::lock_guard<std::mutex> guard(redrawMux);
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

                        default:
                        ke.key = Key::Other;
                    }

                    EventSystem::getGlobalInstance()->queueEvent(ke);

                }
                default:
                break;
            }
        }


        // Lock the redraw mutex and redraw any renders that have requested it
        {
            std::lock_guard<std::mutex> lock(redrawMux);

            for (SDL_Renderer* renderer : toRedraw)
            {

                if (renderStack.count(renderer) == 0)
                {
                    Log::writeToLog(Log::ERR, "Redraw requested for nonexistent renderer ", renderer, "!");
                    throw std::runtime_error("Redraw requested for nonexistent renderer!");
                }
                Log::writeToLog(Log::L_DEBUG, "Redrawing renderer ", renderer);

                for (Renderable* renderable : renderStack[renderer])
                {
                    renderable->redraw();
                }
            }
            // Clear the redraw queue; we're done!
            toRedraw.clear();
        }
                

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    }
}



