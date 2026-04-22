#include "label.hpp"

Label::Label(std::string text, ivec2 pos, vec3 col) : m_text(text), m_position(pos), m_color(col) 
{
}

std::string Label::getText() const
{
    return m_text;
}

void Label::setText(std::string text)
{
    m_text = std::move(text);
}

void Label::draw(Screen& screen) 
{
    int charWidth = 8; 
    int charHeight = 12;

    for (size_t i = 0; i < m_text.length(); ++i) 
    {
        int xOffset = static_cast<int>(i) * charWidth;
        ivec2 charPos = { m_position.x + static_cast<int>(i * charWidth), m_position.y };
        ivec2 charMax = { charPos.x + charWidth - 2, charPos.y + charHeight };

        screen.drawBox(charPos, charMax, m_color); // ask how to implement font
    }
}