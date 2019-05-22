#pragma once

#include "../common/EventSystem.h"
#include "../common/SimulationEvents.h"
#include "MockUIEvents.h"
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

    HandleResult handleTeamEvent(TeamOwnership* event);

private:
    SDL_AudioDeviceID outputDeviceID;
    SDL_AudioSpec outputDeviceSpec;

    std::vector<uint8_t> loadVoice(const char *filename);
    void playVoice(const std::vector<uint8_t> *voice);

    std::vector<uint8_t> voiceGameStart;

    std::vector<uint8_t> voiceOwnFlagTaken;
    std::vector<uint8_t> voiceEnemyFlagTaken;

    std::vector<uint8_t> voiceOwnFlagScored;
    std::vector<uint8_t> voiceEnemyFlagScored;

    std::vector<uint8_t> voiceOwnSubKill;
    std::vector<uint8_t> voiceEnemySubKill;

    std::vector<uint8_t> voiceOwnFlagSubKill;
    std::vector<uint8_t> voiceEnemyFlagSubKill;

    uint32_t team;

    HandleResult handleStatusUpdate(StatusUpdateEvent *event);
};
