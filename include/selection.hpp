#ifndef __SELECTION_HPP__
#define __SELECTION_HPP__

#include "Cloneable.hpp"
#include "linear.hpp"
#include <string>

class Selection : public Cloneable<Selection>
{
private:
    ivec2 m_position;
    std::string m_label;
    bool m_isSelected;
    static constexpr int m_boxSize = 20; // currently hard coded

public:
    Selection(ivec2 position, std::string label, bool checked = false);

    virtual ~Selection() = default;

    virtual void draw(Screen& screen) override;

    void toggle();
    bool isSelected() const;
};

#endif
