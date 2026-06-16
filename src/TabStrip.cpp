#include "TabStrip.hpp"
#include "ClickEvent.hpp"
#include "font.hpp"

// Approximate per-glyph advance used for centering the label inside its
// cell. Font::drawText writes a TTF surface whose true width we don't get
// to query from here, so we estimate it from the character count. Six
// pixels per glyph matches the body face used by Label well enough to
// keep tab text visually centered without bleeding past the separators.
static constexpr int APPROX_GLYPH_WIDTH = 6;
static constexpr int APPROX_GLYPH_HEIGHT = 12;

TabStrip::TabStrip(ivec2 minP, ivec2 maxP,
                   vec3 tabBg, vec3 tabBgActive,
                   vec3 textColor, vec3 separatorColor)
    : minPos(minP), maxPos(maxP), labels(),
      selectedIndex(0),
      tabBg(tabBg), tabBgActive(tabBgActive),
      textColor(textColor), separatorColor(separatorColor)
{
}

void TabStrip::addTab(const std::string& label)
{
    labels.push_back(label);
}

void TabStrip::setSelected(std::size_t index)
{
    if (labels.empty())
    {
        return;
    }
    if (index >= labels.size())
    {
        return;
    }
    selectedIndex = index;
}

std::size_t TabStrip::getSelected() const
{
    return selectedIndex;
}

std::size_t TabStrip::getTabCount() const
{
    return labels.size();
}

void TabStrip::draw(Screen& screen)
{
    if (labels.empty())
    {
        return;
    }

    int stripWidth = maxPos.x - minPos.x;
    int tabCount = static_cast<int>(labels.size());
    if (stripWidth <= 0 || tabCount <= 0)
    {
        return;
    }

    int tabWidth = stripWidth / tabCount;
    if (tabWidth <= 0)
    {
        return;
    }

    int stripHeight = maxPos.y - minPos.y;

    for (int i = 0; i < tabCount; ++i)
    {
        ivec2 cellMin(minPos.x + i * tabWidth, minPos.y);
        int rightEdge;
        if (i == tabCount - 1)
        {
            rightEdge = maxPos.x;
        }
        else
        {
            rightEdge = minPos.x + (i + 1) * tabWidth;
        }
        ivec2 cellMax(rightEdge, maxPos.y);

        const vec3& bg = (static_cast<std::size_t>(i) == selectedIndex) ? tabBgActive : tabBg;
        screen.drawBox(cellMin, cellMax, bg);

        // 1px vertical separator between adjacent tabs. Drawn on the right
        // edge of every tab except the last so the strip ends flush with
        // maxPos.x rather than with a stray divider hanging off the end.
        if (i < tabCount - 1)
        {
            ivec2 sepMin(cellMax.x - 1, minPos.y);
            ivec2 sepMax(cellMax.x, maxPos.y);
            screen.drawBox(sepMin, sepMax, separatorColor);
        }

        const std::string& label = labels[static_cast<std::size_t>(i)];
        int labelPxWidth = static_cast<int>(label.size()) * APPROX_GLYPH_WIDTH;
        int cellInnerWidth = cellMax.x - cellMin.x;
        int textX = cellMin.x + (cellInnerWidth - labelPxWidth) / 2;
        if (textX < cellMin.x + 1)
        {
            textX = cellMin.x + 1;
        }
        int textY = cellMin.y + (stripHeight - APPROX_GLYPH_HEIGHT) / 2;
        if (textY < cellMin.y)
        {
            textY = cellMin.y;
        }
        Font::drawText(screen, label, ivec2(textX, textY), textColor);
    }
}

bool TabStrip::operator()(Event* event)
{
    if (event->getType() != EVENT_CLICK)
    {
        return false;
    }
    if (labels.empty())
    {
        return false;
    }

    ClickEvent* click = static_cast<ClickEvent*>(event);
    int mx = click->getMouseX();
    int my = click->getMouseY();

    if (mx < minPos.x || mx > maxPos.x)
    {
        return false;
    }
    if (my < minPos.y || my > maxPos.y)
    {
        return false;
    }

    int stripWidth = maxPos.x - minPos.x;
    int tabCount = static_cast<int>(labels.size());
    if (stripWidth <= 0 || tabCount <= 0)
    {
        return false;
    }

    int tabWidth = stripWidth / tabCount;
    if (tabWidth <= 0)
    {
        return false;
    }

    int offset = mx - minPos.x;
    int hit = offset / tabWidth;
    if (hit < 0)
    {
        hit = 0;
    }
    if (hit >= tabCount)
    {
        hit = tabCount - 1;
    }

    selectedIndex = static_cast<std::size_t>(hit);
    return true;
}
