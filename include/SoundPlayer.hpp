#ifndef __SOUNDPLAYER_HPP__
#define __SOUNDPLAYER_HPP__

#include <string>
#include <SDL3/SDL.h>

class SoundPlayer
{
private:
    SDL_AudioStream* stream;

    SoundPlayer();
    SoundPlayer(const SoundPlayer&) = delete;
    SoundPlayer& operator=(const SoundPlayer&) = delete;

public:
    ~SoundPlayer();

    static SoundPlayer& getInstance();

    void play(const std::string& filePath);
};

#endif
