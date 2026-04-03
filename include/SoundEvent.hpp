#ifndef __SOUNDEVENT_HPP__
#define __SOUNDEVENT_HPP__

#include <string>
#include "Event.hpp"

class SoundEvent : public Event
{
private:
    std::string filePath;

public:
    SoundEvent(const std::string& filePath);
    const std::string& getFilePath() const;
};

#endif
