#ifndef __STACKED_BAR_HPP__
#define __STACKED_BAR_HPP__

#include <utility>
#include <vector>
#include "Cloneable.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// StackedBar : horizontal bar partitioned into N colored segments whose
// values sum to a configurable total. Pure-additive widget that reuses
// the existing Screen::drawBox primitive and shares the Cloneable CRTP
// plumbing with Bar, Sparkline, and friends; the value semantics differ
// only in that the strip is composed of stacked slices rather than a
// single fill ratio.
class StackedBar : public Cloneable<StackedBar>
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    std::vector<std::pair<float, vec3>> segments;
    float total;
    vec3 bgColor;

public:
    StackedBar(ivec2 minPos, ivec2 maxPos, vec3 bgColor, float total = 100.0f);

    void addSegment(float value, vec3 color);
    void clearSegments();
    void setTotal(float total);

    float getTotal() const;
    std::size_t getSegmentCount() const;
    ivec2 getMinPos() const;
    ivec2 getMaxPos() const;

    void draw(Screen &screen) override;
};

#endif
