#ifndef __TOOLTIP_HPP__
#define __TOOLTIP_HPP__

#include "Cloneable.hpp"
#include "linear.hpp"
#include <string>

// Tooltip is a small floating label drawn on top of the rest of the scene.
// The widget itself is dumb: visibility is owned by whoever holds the tooltip
// (e.g., a hover handler) and we just no-op draw when visible == false. The
// label sits at pos with a small padding, a filled bgColor body, a 1px
// borderColor outline, and textColor glyphs through the shared SDL debug
// font. Width is estimated from text.size() * 8 because the debug font is
// roughly 8px per glyph and we do not pull the actual glyph metrics on the
// hot path.
class Tooltip : public Cloneable<Tooltip>
{
public:
    ivec2 pos;
    std::string text;
    bool visible;
    vec3 bgColor;
    vec3 borderColor;
    vec3 textColor;
    int padding;

    Tooltip();
    Tooltip(ivec2 pos, std::string text);

    virtual ~Tooltip() = default;

    void setText(std::string text);
    void setPos(ivec2 pos);
    void setVisible(bool visible);
    bool isVisible() const;

    virtual void draw(Screen& screen) override;
};

#endif
