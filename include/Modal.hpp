#ifndef __MODAL_HPP__
#define __MODAL_HPP__

#include <string>
#include "Cloneable.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Modal is a centered dialog widget rendered on top of a translucent overlay
// that covers the full screen. Owner controls visibility via setVisible. When
// hidden, draw is a no-op. Body text is treated as a single line; multi-line
// wrap is out of scope for this widget.
class Modal : public Cloneable<Modal>
{
private:
    int screenWidth;
    int screenHeight;
    int dialogWidth;
    int dialogHeight;
    std::string title;
    std::string body;
    bool visible;
    vec3 overlayColor;
    vec3 dialogBg;
    vec3 dialogBorder;
    vec3 titleColor;
    vec3 bodyColor;

public:
    Modal(int screenWidth, int screenHeight, int dialogWidth, int dialogHeight,
          std::string title, std::string body,
          vec3 overlayColor, vec3 dialogBg, vec3 dialogBorder,
          vec3 titleColor, vec3 bodyColor);

    void setTitle(std::string title);
    void setBody(std::string body);
    void setVisible(bool visible);
    bool isVisible() const;

    void draw(Screen &screen) override;
};

#endif
