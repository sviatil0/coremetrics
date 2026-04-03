#include "ShowEvent.hpp"

ShowEvent::ShowEvent(const std::string& layoutName, bool show) : Event(EVENT_SHOW), layoutName(layoutName), show(show)
{
}

const std::string& ShowEvent::getLayoutName() const
{
    return layoutName;
}

bool ShowEvent::getShow() const
{
    return show;
}
