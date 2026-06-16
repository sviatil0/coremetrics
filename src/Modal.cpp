#include "Modal.hpp"
#include "font.hpp"

// Title sits a small constant inset below the top edge of the dialog so the
// glyph baseline does not collide with the 1px border. Body starts a row
// below the title using a fixed line height that matches the regular font
// metric used elsewhere in the UI (Font ships 20pt Roboto Bold).
constexpr int MODAL_TITLE_INSET_Y = 8;
constexpr int MODAL_TEXT_INSET_X = 12;
constexpr int MODAL_LINE_HEIGHT = 22;

Modal::Modal(int screenWidth, int screenHeight, int dialogWidth, int dialogHeight,
             std::string title, std::string body,
             vec3 overlayColor, vec3 dialogBg, vec3 dialogBorder,
             vec3 titleColor, vec3 bodyColor)
    : screenWidth(screenWidth), screenHeight(screenHeight),
      dialogWidth(dialogWidth), dialogHeight(dialogHeight),
      title(std::move(title)), body(std::move(body)), visible(false),
      overlayColor(overlayColor), dialogBg(dialogBg), dialogBorder(dialogBorder),
      titleColor(titleColor), bodyColor(bodyColor)
{
}

void Modal::setTitle(std::string newTitle)
{
    title = std::move(newTitle);
}

void Modal::setBody(std::string newBody)
{
    body = std::move(newBody);
}

void Modal::setVisible(bool newVisible)
{
    visible = newVisible;
}

bool Modal::isVisible() const
{
    return visible;
}

void Modal::draw(Screen &screen)
{
    if (!visible)
    {
        return;
    }

    // Full-screen translucent overlay. Screen::drawBox does not blend, so the
    // caller picks a near-black overlayColor that visually dims the content
    // underneath when written opaque.
    ivec2 overlayMin(0, 0);
    ivec2 overlayMax(screenWidth, screenHeight);
    screen.drawBox(overlayMin, overlayMax, overlayColor);

    // Center the dialog rect inside the screen. Integer division here is
    // intentional: pixel coords must be whole, and a 1px bias is invisible.
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    ivec2 dialogMin(dialogX, dialogY);
    ivec2 dialogMax(dialogX + dialogWidth, dialogY + dialogHeight);

    screen.drawBox(dialogMin, dialogMax, dialogBg);

    // 1px border drawn as four thin strips (top, bottom, left, right) so the
    // dialogBg fill stays untouched in the interior. drawBox is the only
    // primitive used; no new rasterizer paths.
    ivec2 topMin(dialogMin.x, dialogMin.y);
    ivec2 topMax(dialogMax.x, dialogMin.y + 1);
    screen.drawBox(topMin, topMax, dialogBorder);

    ivec2 bottomMin(dialogMin.x, dialogMax.y - 1);
    ivec2 bottomMax(dialogMax.x, dialogMax.y);
    screen.drawBox(bottomMin, bottomMax, dialogBorder);

    ivec2 leftMin(dialogMin.x, dialogMin.y);
    ivec2 leftMax(dialogMin.x + 1, dialogMax.y);
    screen.drawBox(leftMin, leftMax, dialogBorder);

    ivec2 rightMin(dialogMax.x - 1, dialogMin.y);
    ivec2 rightMax(dialogMax.x, dialogMax.y);
    screen.drawBox(rightMin, rightMax, dialogBorder);

    // Title at the top of the dialog, body one line below. Both use the
    // shared Font::drawText path so the modal inherits whatever font the
    // rest of the UI already loaded.
    ivec2 titlePos(dialogMin.x + MODAL_TEXT_INSET_X, dialogMin.y + MODAL_TITLE_INSET_Y);
    Font::drawText(screen, title, titlePos, titleColor);

    ivec2 bodyPos(dialogMin.x + MODAL_TEXT_INSET_X, dialogMin.y + MODAL_TITLE_INSET_Y + MODAL_LINE_HEIGHT);
    Font::drawText(screen, body, bodyPos, bodyColor);
}
