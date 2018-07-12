#include "UI.h"

#include "../common/Log.h"
#include "../common/Exceptions.h"

#include "SDL.h"

/// Static definition of singleton
UI* UI::singleton = nullptr;

UI::UI()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        Log::writeToLog(Log::ERR, "Couldn't start SDL! SDL error:", SDL_GetError());
        throw SDLError("Error in SDL_Init");
    }
}

UI::~UI()
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
    
