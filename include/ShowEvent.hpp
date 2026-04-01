#ifndef __SHOWEVENT_HPP__
#define __SHOWEVENT_HPP__

#include <string>
#include "Event.hpp"

class ShowEvent : public Event
{
private:
    std::string layoutName;
    bool show;

public:
    ShowEvent(const std::string& layoutName, bool show);
    const std::string& getLayoutName() const;
    bool getShow() const;
};

#endif
