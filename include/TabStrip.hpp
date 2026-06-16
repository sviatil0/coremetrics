#ifndef __TABSTRIP_HPP__
#define __TABSTRIP_HPP__

#include <cstddef>
#include <string>
#include <vector>
#include "Cloneable.hpp"
#include "linear.hpp"

// TabStrip: a horizontal row of N text tabs with exactly one tab marked
// active. The strip occupies the rectangle [minPos, maxPos] and divides
// that width evenly across labels.size() cells. Clicking inside the strip
// recomputes the active index from the x coordinate and updates state in
// place. The widget is a pure presentation/input control; it does not
// dispatch ShowEvent or any cross-layout signal on selection change.
//
// Author: Sviatoslav Oleksiienko <soleksiienko1@gmail.com>
class TabStrip : public Cloneable<TabStrip>
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    std::vector<std::string> labels;
    std::size_t selectedIndex;
    vec3 tabBg;
    vec3 tabBgActive;
    vec3 textColor;
    vec3 separatorColor;

public:
    TabStrip(ivec2 minP, ivec2 maxP,
             vec3 tabBg, vec3 tabBgActive,
             vec3 textColor, vec3 separatorColor);

    ~TabStrip() {}

    void addTab(const std::string& label);
    void setSelected(std::size_t index);
    std::size_t getSelected() const;
    std::size_t getTabCount() const;

    void draw(Screen& screen) override;
    bool operator()(Event* event) override;
};

#endif
