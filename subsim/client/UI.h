#pragma once

#include <vector>

#include <cstdint>

/// Forward declaration of SDL_Renderer
class SDL_Renderer;

/// Forward declaration of SDL_Window
class SDL_Window;

/**
 * This class handles setting up SDL windows; it is a form of a window manager
 * in addition to handling user input.
 *
 * This includes creating/destroying windows, handing out window contexts and such.
 *
 * This also includes static memberes for setting a "singleton" that can be retrieved by
 * other classes, without explicitly passing around references to this.
 */
class UI
{
    public:
    /// Inits SDL
    UI();

    /// Shuts down SDL properly
    ~UI();

    /// Remove the copy constructor
    UI(const UI& ui) = delete;

    /// And remove the move constructor
    UI(UI&& ui) = delete;

    /// Remove the copy assignment operator
    UI& operator=(const UI& other) = delete;

    /// and remove the move assignment operator
    UI&& operator=(UI&& other) = delete;

    /// Gets the singleton pointer to this class. Throws an exception if no singleton has been set
    static UI* getGlobalUI();

    /// Sets the singleton pointer to this class
    static void setGlobalUI(UI* singleton_);
    
    /// Returns an SDL_Renderer that is at least as big as the specified size
    SDL_Renderer* getFreeRenderer(uint16_t minWidth, uint16_t minHeight);

private:
    static UI* singleton;

    /// Stores allocated windows
    std::vector<SDL_Window*> windows;

    /// Stores allocated renderers
    std::vector<SDL_Renderer*> renderers;
};
