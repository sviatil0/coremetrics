#ifndef __LABEL_HPP__
#define __LABEL_HPP__

#include "Cloneable.hpp"
#include "linear.hpp"
#include <string>

class Label : public Cloneable<Label>
{
private:
    std::string m_text;
    ivec2 m_position;
    vec3 m_color;

public:
    Label(std::string text, ivec2 position, vec3 color);

    virtual ~Label() = default;

    virtual void draw(Screen& screen) override;

    std::string getText() const;
    void setText(std::string text);
    void setPos(ivec2 pos);
    void setColor(vec3 color);
};

#endif
