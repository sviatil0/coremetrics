#include "SoundPlayer.hpp"
#include <iostream>

constexpr int SAMPLE_RATE = 44100;
constexpr int NUM_CHANNELS = 1;

SoundPlayer::SoundPlayer() : stream(nullptr)
{
}

SoundPlayer::~SoundPlayer()
{
    if (stream != nullptr)
    {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
}

SoundPlayer& SoundPlayer::getInstance()
{
    static SoundPlayer instance;
    return instance;
}

void SoundPlayer::play(const std::string& filePath)
{
    SDL_AudioSpec wavSpec;
    Uint8* wavBuffer = nullptr;
    Uint32 wavLength = 0;

    if (!SDL_LoadWAV(filePath.c_str(), &wavSpec, &wavBuffer, &wavLength))
    {
        std::cerr << "Failed to load WAV: " << SDL_GetError() << '\n';
        return;
    }

    SDL_AudioSpec desiredSpec;
    desiredSpec.freq = SAMPLE_RATE;
    desiredSpec.format = SDL_AUDIO_F32;
    desiredSpec.channels = NUM_CHANNELS;

    if (stream != nullptr)
    {
        SDL_DestroyAudioStream(stream);
    }

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desiredSpec, nullptr, nullptr);
    if (stream == nullptr)
    {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << '\n';
        SDL_free(wavBuffer);
        return;
    }

    SDL_PutAudioStreamData(stream, wavBuffer, wavLength);
    SDL_ResumeAudioStreamDevice(stream);
    SDL_free(wavBuffer);
}
