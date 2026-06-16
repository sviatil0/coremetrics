#ifndef __TOGGLE_HPP__
#define __TOGGLE_HPP__

#include "Cloneable.hpp"
#include "linear.hpp"

// Toggle is a pill-shaped on/off switch with a sliding knob. When clicked
// inside its bounding box, it flips its internal state and reports the
// click as handled. The pill body is drawn with onColor when state is
// true and offColor otherwise; the knob is the same size on both sides
// and slides between the right (on) and left (off) inset positions.
class Toggle : public Cloneable<Toggle>
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    bool state;
    vec3 onColor;
    vec3 offColor;
    vec3 knobColor;

public:
    Toggle(ivec2 minPos, ivec2 maxPos, bool initialState,
           vec3 onColor, vec3 offColor, vec3 knobColor);

    ~Toggle() override {}

    bool getState() const;
    void setState(bool newState);

    void draw(Screen& screen) override;

    bool checkToggle(int mouseX, int mouseY) const;

    bool operator()(Event* event) override;
};

#endif
