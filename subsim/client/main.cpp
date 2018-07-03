#include <iostream>

#include "version.h"

#include "../common/Network.h"
#include "../common/Log.h"

#include "SDL.h"

void print_usage(char* prog_name)
{
    std::cerr << prog_name << " -s [ip/hostname]\n";
}

int main(int argc, char **argv)
{
    Log::setLogfile(std::string(argv[0]) + ".log");
    Log::clearLog();
    Log::shouldMirrorToConsole(true);
    Log::setLogLevel(Log::ALL);

    Log::writeToLog(Log::INFO, "Subsim client version v", VERSION_MAJOR, ".", VERSION_MINOR, " starting");

    if (argc != 3 || std::string(argv[1]) != std::string("-s"))
    {
        print_usage(argv[0]);
        return 1;
    }

    Network network;
    network.connect(argv[2]);

    // Startup SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_Window *window = SDL_CreateWindow("Lobby", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_Delay(3000);

    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Press enter to exit...\n";
    std::string dummy;
    std::getline(std::cin, dummy);
    return 0;
}
