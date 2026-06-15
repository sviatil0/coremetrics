#include "Label.hpp"
#include "font.hpp"

Label::Label(std::string text, ivec2 pos, vec3 col) : m_text(std::move(text)), m_position(pos), m_color(col)
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

void Label::setPos(ivec2 pos)
{
    m_position = pos;
}

void Label::setColor(vec3 color)
{
    m_color = color;
}

void Label::draw(Screen &screen)
{
    Font::drawText(screen, m_text, m_position, m_color);
}
