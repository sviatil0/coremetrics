#include "selection.hpp"

Selection::Selection(ivec2 pos, std::string label, bool checked) 
    : m_position(pos), m_label(label), m_isSelected(checked) 
{
}

void Selection::toggle() 
{
    m_isSelected = !m_isSelected;
}

bool Selection::isSelected() const 
{
    return m_isSelected;
}

void Selection::draw(Screen& screen) 
{
    ivec2 maxPos = { m_position.x + m_boxSize, m_position.y + m_boxSize };
    vec3 borderColor = { 1.0f, 1.0f, 1.0f }; // White border
    vec3 bgColor = { 0.1f, 0.1f, 0.1f };  // Grey background
    
    screen.drawBox(m_position, maxPos, borderColor);
    
    ivec2 innerMin = { m_position.x + 1, m_position.y + 1 };
    ivec2 innerMax = { maxPos.x - 1, maxPos.y - 1 };
    screen.drawBox(innerMin, innerMax, bgColor);

    if (m_isSelected) 
    {
        vec3 checkColor = { 0.0f, 1.0f, 0.0f };
        ivec2 markMin = { m_position.x + 4, m_position.y + 4 };
        ivec2 markMax = { maxPos.x - 4, maxPos.y - 4 };
        screen.drawBox(markMin, markMax, checkColor);
    }
}