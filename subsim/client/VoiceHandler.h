#pragma once

#include "../common/EventSystem.h"
#include "../common/SimulationEvents.h"
#include "SDL.h"

/*!
 * Class that plays voice messages when important events happen. There should
 * only ever be one instance.
 */
class VoiceHandler : public EventReceiver
{
public:
    VoiceHandler();
    ~VoiceHandler();

private:
    SDL_AudioDeviceID outputDeviceID;
    SDL_AudioSpec outputDeviceSpec;

    std::vector<uint8_t> loadVoice(const char *filename);
    void playVoice(const std::vector<uint8_t> *voice);

    std::vector<uint8_t> voiceGameStart;

    HandleResult handleStatusUpdate(StatusUpdateEvent *event);
};
