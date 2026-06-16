#ifndef __DROPDOWN_HPP__
#define __DROPDOWN_HPP__

// Dropdown widget. Collapsed state shows the currently selected label and
// a small downward caret on the right edge of the bar. A click inside the
// bar toggles the expanded state, which paints a vertical strip below the
// bar with one option per row (22px tall each). Clicking a row commits
// the selection and collapses; clicking anywhere outside collapses
// without changing the selection. The widget is style-only state -- no
// EventManager pushes -- because downstream code can poll getSelected()
// each frame to react to changes.
//
// Author: Sviatoslav Oleksiienko <soleksiienko1@gmail.com>

#include <cstddef>
#include <string>
#include <vector>
#include "Cloneable.hpp"
#include "linear.hpp"

class Dropdown : public Cloneable<Dropdown>
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    std::vector<std::string> options;
    std::size_t selectedIndex;
    bool expanded;
    vec3 bgColor;
    vec3 textColor;
    vec3 hoverBg;

public:
    static constexpr int ROW_HEIGHT = 22;

    Dropdown(ivec2 minP, ivec2 maxP, vec3 bgCol, vec3 textCol, vec3 hoverCol);

    ~Dropdown() override = default;

    void addOption(std::string option);
    void setSelected(std::size_t index);
    std::size_t getSelected() const;
    bool isExpanded() const;
    std::size_t optionCount() const;

    void draw(Screen& screen) override;
    bool operator()(Event* event) override;
};

#endif
