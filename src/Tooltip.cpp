#include "Tooltip.hpp"
#include "font.hpp"

// Glyph width estimate for the shared SDL debug font. The Font module
// does not expose a measure-string call on the hot path, so every tooltip
// uses this conservative fixed advance to size its background rect. Eight
// pixels matches the visual width of the bold debug glyphs used by the
// CPU/RAM/GPU readouts.
static constexpr int GLYPH_WIDTH_PX = 8;

// Single-line glyph height for the shared SDL debug font. The Font module
// renders one line at roughly 16px tall, so the background rect grows
// vertically by GLYPH_HEIGHT_PX plus twice the padding.
static constexpr int GLYPH_HEIGHT_PX = 16;

Tooltip::Tooltip()
    : pos(),
      text(),
      visible(false),
      bgColor(0.1f, 0.1f, 0.1f),
      borderColor(1.0f, 1.0f, 1.0f),
      textColor(1.0f, 1.0f, 1.0f),
      padding(4)
{
}

Tooltip::Tooltip(ivec2 pos, std::string text)
    : pos(pos),
      text(std::move(text)),
      visible(false),
      bgColor(0.1f, 0.1f, 0.1f),
      borderColor(1.0f, 1.0f, 1.0f),
      textColor(1.0f, 1.0f, 1.0f),
      padding(4)
{
}

void Tooltip::setText(std::string newText)
{
    text = std::move(newText);
}

void Tooltip::setPos(ivec2 newPos)
{
    pos = newPos;
}

void Tooltip::setVisible(bool newVisible)
{
    visible = newVisible;
}

bool Tooltip::isVisible() const
{
    return visible;
}

void Tooltip::draw(Screen& screen)
{
    // Visibility is owned by whoever holds this tooltip (hover handler,
    // selection logic, etc.). When hidden we draw nothing at all so the
    // widget composes cleanly under any layout without an explicit
    // skip-list at the layout layer.
    if (!visible)
    {
        return;
    }

    int textWidth = static_cast<int>(text.size()) * GLYPH_WIDTH_PX;
    int boxMaxX = pos.x + textWidth + 2 * padding;
    int boxMaxY = pos.y + GLYPH_HEIGHT_PX + 2 * padding;

    ivec2 boxMin(pos.x, pos.y);
    ivec2 boxMax(boxMaxX, boxMaxY);

    // Filled background body first, then four 1px strips on top of it as
    // the border. Doing the border as four drawBox calls keeps drawBox's
    // own fill semantics intact (drawBox fills, it does not stroke) while
    // still honoring the "1px border in borderColor" requirement.
    screen.drawBox(boxMin, boxMax, bgColor);

    ivec2 topStripMin(boxMin.x, boxMin.y);
    ivec2 topStripMax(boxMax.x, boxMin.y);
    screen.drawBox(topStripMin, topStripMax, borderColor);

    ivec2 bottomStripMin(boxMin.x, boxMax.y);
    ivec2 bottomStripMax(boxMax.x, boxMax.y);
    screen.drawBox(bottomStripMin, bottomStripMax, borderColor);

    ivec2 leftStripMin(boxMin.x, boxMin.y);
    ivec2 leftStripMax(boxMin.x, boxMax.y);
    screen.drawBox(leftStripMin, leftStripMax, borderColor);

    ivec2 rightStripMin(boxMax.x, boxMin.y);
    ivec2 rightStripMax(boxMax.x, boxMax.y);
    screen.drawBox(rightStripMin, rightStripMax, borderColor);

    ivec2 textPos(pos.x + padding, pos.y + padding);
    Font::drawText(screen, text, textPos, textColor);
}
