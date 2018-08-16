#include "UI.h"

#include "../common/Log.h"
#include "../common/Exceptions.h"

#include "SDL.h"

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
    if (renderer != nullptr)
    {
        UI::getGlobalUI()->deregisterRenderable(renderer, this);
    }
}

void Renderable::scheduleRedraw()
{
    if (renderer != nullptr)
    {
        UI::getGlobalUI()->triggerRedraw(renderer);
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
        Log::writeToLog(Log::L_DEBUG, "Registered renderable ", renderable, " on renderer", renderer);
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

    renderStack[renderer].erase(renderable);
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
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(170));

    }
}



