#include "Dropdown.hpp"
#include "ClickEvent.hpp"
#include "font.hpp"

// Construct a collapsed dropdown with no options. selectedIndex starts at
// 0 so that the very first addOption() call lands on a valid display
// state even if the caller never calls setSelected() explicitly.
Dropdown::Dropdown(ivec2 minP, ivec2 maxP, vec3 bgCol, vec3 textCol, vec3 hoverCol)
    : minPos(minP), maxPos(maxP), options(), selectedIndex(0), expanded(false),
      bgColor(bgCol), textColor(textCol), hoverBg(hoverCol)
{
}

void Dropdown::addOption(std::string option)
{
    options.push_back(std::move(option));
}

// Guard against out-of-range writes so callers cannot put the widget
// into a state where getSelected() points past the option list. Calling
// setSelected on an empty list is a no-op.
void Dropdown::setSelected(std::size_t index)
{
    if (options.empty())
    {
        return;
    }
    if (index >= options.size())
    {
        return;
    }
    selectedIndex = index;
}

std::size_t Dropdown::getSelected() const
{
    return selectedIndex;
}

bool Dropdown::isExpanded() const
{
    return expanded;
}

std::size_t Dropdown::optionCount() const
{
    return options.size();
}

// Paint pass: always draw the collapsed bar with a 1px white border, the
// current label inset 4px from the left, and a 6px downward caret on
// the right (drawn as a row of horizontal lines that shrink by one
// pixel per row, which is cheap and avoids needing drawTriangle's
// barycentric path for such a tiny shape). When expanded, paint a
// vertical strip below the bar, one row per option, with the currently
// selected row tinted with hoverBg so the user can see which row their
// click landed on after collapse.
void Dropdown::draw(Screen& screen)
{
    vec3 border(1.0f, 1.0f, 1.0f);
    screen.drawBox(minPos, maxPos, border);
    ivec2 innerMin(minPos.x + 1, minPos.y + 1);
    ivec2 innerMax(maxPos.x - 1, maxPos.y - 1);
    screen.drawBox(innerMin, innerMax, bgColor);

    if (!options.empty() && selectedIndex < options.size())
    {
        ivec2 textPos(minPos.x + 4, minPos.y + 4);
        Font::drawText(screen, options[selectedIndex], textPos, textColor);
    }

    int caretRight = maxPos.x - 4;
    int caretLeft = caretRight - 6;
    int caretMidY = (minPos.y + maxPos.y) / 2 - 2;
    for (int row = 0; row < 4; row++)
    {
        ivec2 a(caretLeft + row, caretMidY + row);
        ivec2 b(caretRight - row, caretMidY + row);
        screen.drawLine(a, b, textColor);
    }

    if (!expanded)
    {
        return;
    }

    int barHeight = maxPos.y - minPos.y;
    int stripTop = maxPos.y + 1;
    int stripBottom = stripTop + static_cast<int>(options.size()) * ROW_HEIGHT;
    ivec2 stripMin(minPos.x, stripTop);
    ivec2 stripMax(maxPos.x, stripBottom);
    screen.drawBox(stripMin, stripMax, border);
    ivec2 stripInnerMin(stripMin.x + 1, stripMin.y + 1);
    ivec2 stripInnerMax(stripMax.x - 1, stripMax.y - 1);
    screen.drawBox(stripInnerMin, stripInnerMax, bgColor);

    for (std::size_t i = 0; i < options.size(); i++)
    {
        int rowTop = stripTop + static_cast<int>(i) * ROW_HEIGHT;
        int rowBottom = rowTop + ROW_HEIGHT;
        if (i == selectedIndex)
        {
            ivec2 hlMin(stripInnerMin.x, rowTop);
            ivec2 hlMax(stripInnerMax.x, rowBottom);
            screen.drawBox(hlMin, hlMax, hoverBg);
        }
        ivec2 rowTextPos(minPos.x + 4, rowTop + (ROW_HEIGHT - barHeight) / 2 + 4);
        Font::drawText(screen, options[i], rowTextPos, textColor);
    }
}

// Click router. Three regions matter: the collapsed bar (toggles
// expand), the expanded option strip (commits selection + collapses),
// and everywhere else (collapses with no change). Returning true means
// the click was consumed, which lets the EventManager skip widgets
// stacked underneath us.
bool Dropdown::operator()(Event* event)
{
    if (event == nullptr)
    {
        return false;
    }
    if (event->getType() != EVENT_CLICK)
    {
        return false;
    }

    ClickEvent* click = static_cast<ClickEvent*>(event);
    int mx = click->getMouseX();
    int my = click->getMouseY();

    bool inBar = (mx >= minPos.x) && (mx <= maxPos.x)
              && (my >= minPos.y) && (my <= maxPos.y);

    if (inBar)
    {
        expanded = !expanded;
        return true;
    }

    if (expanded && !options.empty())
    {
        int stripTop = maxPos.y + 1;
        int stripBottom = stripTop + static_cast<int>(options.size()) * ROW_HEIGHT;
        bool inStrip = (mx >= minPos.x) && (mx <= maxPos.x)
                    && (my >= stripTop) && (my < stripBottom);
        if (inStrip)
        {
            int offset = my - stripTop;
            std::size_t idx = static_cast<std::size_t>(offset / ROW_HEIGHT);
            if (idx < options.size())
            {
                selectedIndex = idx;
            }
            expanded = false;
            return true;
        }
    }

    if (expanded)
    {
        expanded = false;
        return true;
    }

    return false;
}
