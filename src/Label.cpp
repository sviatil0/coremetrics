<<<<<<< HEAD:src/Label.cpp
#include "Label.hpp"
=======
#include "label.hpp"
#include "font.hpp"
>>>>>>> 4f9eeea89c76feba4db6dc7aad8a164f3b50c71d:src/label.cpp

Label::Label(std::string text, ivec2 pos, vec3 col) : m_text(std::move(text)), m_position(pos), m_color(col)
{
}

std::string Label::getText() const
{
    return m_text;
}

void Label::setText(std::string text)
<<<<<<< HEAD:src/Label.cpp
{
    this->m_text = text;
}

/*void Label::draw(Screen& screen) 
=======
>>>>>>> 4f9eeea89c76feba4db6dc7aad8a164f3b50c71d:src/label.cpp
{
    m_text = std::move(text);
}

<<<<<<< HEAD:src/Label.cpp
    for (size_t i = 0; i < m_text.length(); ++i) 
    {
        int xOffset = static_cast<int>(i) * charWidth;
        ivec2 charPos = { m_position.x + static_cast<int>(i * charWidth), m_position.y };
        ivec2 charMax = { charPos.x + charWidth - 2, charPos.y + charHeight };

        screen.drawBox(charPos, charMax, m_color); // ask how to implement font
    }
}*/

void Label::draw(Screen& screen)
{
    screen.drawText(this->m_position, this->m_color, this->m_text);
}
=======
void Label::draw(Screen &screen)
{
    Font::drawText(screen, m_text, m_position, m_color);
}
>>>>>>> 4f9eeea89c76feba4db6dc7aad8a164f3b50c71d:src/label.cpp
