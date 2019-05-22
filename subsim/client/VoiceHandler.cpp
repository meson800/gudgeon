#include "VoiceHandler.h"

#include "../common/Exceptions.h"

VoiceHandler::VoiceHandler()
    : EventReceiver({
        dispatchEvent<VoiceHandler, StatusUpdateEvent, &VoiceHandler::handleStatusUpdate>,
        dispatchEvent<VoiceHandler, TeamOwnership, &VoiceHandler::handleTeamEvent>,
        })
{
    SDL_AudioSpec desiredSpec;
    SDL_memset(&desiredSpec, 0, sizeof(desiredSpec));
    desiredSpec.freq = 44100;
    desiredSpec.format = AUDIO_S16;
    desiredSpec.channels = 1;
    desiredSpec.samples = 4096;

    outputDeviceID = SDL_OpenAudioDevice(
        NULL, 0,
        &desiredSpec, &outputDeviceSpec,
        SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (outputDeviceID == 0)
    {
        throw SDLError("SDL_OpenAudioDevice");
    }
    SDL_PauseAudioDevice(outputDeviceID, 0); // unpause

    voiceGameStart = loadVoice("data/sounds/gameStart.wav");

    voiceOwnFlagTaken = loadVoice("data/sounds/flagTakenUs.wav");
    voiceEnemyFlagTaken = loadVoice("data/sounds/flagTakenEnemy.wav");
    voiceOwnFlagScored = loadVoice("data/sounds/flagScoredUs.wav");
    voiceEnemyFlagScored = loadVoice("data/sounds/flagScoredEnemy.wav");
    voiceOwnSubKill = loadVoice("data/sounds/subKillUs.wav");
    voiceEnemySubKill = loadVoice("data/sounds/subKillEnemy.wav");
    voiceOwnFlagSubKill = loadVoice("data/sounds/flagSubKillUs.wav");
    voiceEnemyFlagSubKill = loadVoice("data/sounds/flagSubKillEnemy.wav");
}

VoiceHandler::~VoiceHandler()
{
    SDL_CloseAudioDevice(outputDeviceID);
}

std::vector<uint8_t> VoiceHandler::loadVoice(const char *filename)
{
    // Load the specified .WAV file
    SDL_AudioSpec wavSpec;
    uint8_t *wavBuffer;
    uint32_t wavLength;
    if (SDL_LoadWAV(filename, &wavSpec, &wavBuffer, &wavLength) == NULL)
    {
        throw SDLError("SDL_LoadWAV");
    }

    // Convert it into the same format we're using for output
    SDL_AudioCVT cvt;
    int conversionNeeded = SDL_BuildAudioCVT(
        &cvt,
        wavSpec.format,
        wavSpec.channels,
        wavSpec.freq,
        outputDeviceSpec.format,
        outputDeviceSpec.channels,
        outputDeviceSpec.freq);
    if (conversionNeeded)
    {
        cvt.len = wavLength;
        std::vector<uint8_t> buf(cvt.len * cvt.len_mult);
        memcpy(buf.data(), wavBuffer, wavLength);
        SDL_FreeWAV(wavBuffer);
        cvt.buf = buf.data();
        if (SDL_ConvertAudio(&cvt) < 0)
        {
            throw SDLError("SDL_ConvertAudio");
        }
        buf.resize(cvt.len_cvt);
        return buf;
    }
    else
    {
        std::vector<uint8_t> buf(wavLength);
        memcpy(buf.data(), wavBuffer, wavLength);
        SDL_FreeWAV(wavBuffer);
        return buf;
    }
}

void VoiceHandler::playVoice(const std::vector<uint8_t> *voice)
{
    if (SDL_QueueAudio(outputDeviceID, voice->data(), voice->size()) < 0)
    {
        throw SDLError("SDL_QueueAudio");
    }
}

HandleResult VoiceHandler::handleTeamEvent(TeamOwnership* event)
{
    team = event->team;
    return HandleResult::Stop;
}

HandleResult VoiceHandler::handleStatusUpdate(StatusUpdateEvent *event)
{
    switch (event->type)
    {
        case StatusUpdateEvent::GameStart:
            playVoice(&voiceGameStart);
            break;

        case StatusUpdateEvent::FlagTaken:
            if (event->team != team)
            {
               playVoice(&voiceEnemyFlagTaken);
            }
            else
            {
                playVoice(&voiceOwnFlagTaken);
            }
            break;

        case StatusUpdateEvent::FlagScored:
            if (event->team != team)
            {
               playVoice(&voiceEnemyFlagScored);
            }
            else
            {
                playVoice(&voiceOwnFlagScored);
            }
            break;

        case StatusUpdateEvent::SubKill:
            if (event->team != team)
            {
               playVoice(&voiceEnemySubKill);
            }
            else
            {
                playVoice(&voiceOwnSubKill);
            }
            break;

        case StatusUpdateEvent::FlagSubKill:
            if (event->team != team)
            {
               playVoice(&voiceEnemyFlagSubKill);
            }
            else
            {
                playVoice(&voiceOwnFlagSubKill);
            }
            break;

        default:
            break;
    }
    return HandleResult::Continue;
}

