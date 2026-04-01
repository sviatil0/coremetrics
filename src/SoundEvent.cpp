#include "SoundEvent.hpp"

SoundEvent::SoundEvent(const std::string& filePath) : Event(EVENT_SOUND), filePath(filePath)
{
}

const std::string& SoundEvent::getFilePath() const
{
    return filePath;
}
