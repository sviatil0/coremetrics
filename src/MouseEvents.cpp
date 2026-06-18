#include "MouseEvents.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "ClickEvent.hpp"
#include "EventManager.hpp"
#include "Label.hpp"
#include "Layout.hpp"
#include "LayoutManager.hpp"
#include "ProcessUtils.hpp"
#include "Row.hpp"
#include "Tree.hpp"
#include "font.hpp"
#include "vec2.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 14 of the
// modernization roadmap, mirroring the KeyboardEvents slice. The
// previous in-place block lived inside the SDL_PollEvent switch and
// dispatched every mouse interaction the live UI reacts to. It read
// and wrote a long list of file-scope globals (geometry rects, mute
// flag, sort state) plus a handful of small helpers; this TU keeps
// the same data ownership and consults the rest via extern.
//
// Globals that moved here from coremetrics.cpp so the tests binary
// (which links every src/*.cpp object but not the coremetrics.cpp TU)
// resolves the symbols MouseEvents references. coremetrics.cpp now
// declares each of them extern at the top of the file.
//   g_exitBtnMin, g_exitBtnMax,
//   g_muteBtnMin, g_muteBtnMax,
//   g_systemTabBtnMin, g_systemTabBtnMax,
//   g_processesTabBtnMin, g_processesTabBtnMax,
//   g_aboutTabBtnMin, g_aboutTabBtnMax,
//   g_headerColMin[5], g_headerColMax[5],
//   g_alarmEnabled, g_sortColumn, g_sortAscending,
//   g_muteLabel, g_headerRow, g_shutdownRequested.
// The helpers activateTab(), recenterMuteLabel(), and
// updateHeaderSortGlyph() moved here too; recenterMuteLabel() and
// updateHeaderSortGlyph() are also called from buildScene() in
// coremetrics.cpp, which now consults them via forward declarations.
// Future slices (C9 EventHandlers) will continue tightening this
// surface.

// Geometry rects. Initialized in coremetrics.cpp's buildScene() once
// the scene tree is loaded; consumed here as hit-test bounds.
ivec2 g_exitBtnMin;
ivec2 g_exitBtnMax;
ivec2 g_muteBtnMin;
ivec2 g_muteBtnMax;
ivec2 g_systemTabBtnMin = ivec2(8, 8);
ivec2 g_systemTabBtnMax = ivec2(272, 40);
ivec2 g_processesTabBtnMin = ivec2(280, 8);
ivec2 g_processesTabBtnMax = ivec2(544, 40);
ivec2 g_aboutTabBtnMin = ivec2(552, 8);
ivec2 g_aboutTabBtnMax = ivec2(804, 40);
ivec2 g_headerColMin[5];
ivec2 g_headerColMax[5];

// Sound / alarm + sort flags. The mouse handler is the user-driven
// writer; the renderer, sort comparator, and Settings save/load all
// read them. The mute toggle button writes g_alarmEnabled; the
// header click writes g_sortColumn / g_sortAscending.
bool g_alarmEnabled = true;
SortColumn g_sortColumn = SORT_MEM;
bool g_sortAscending = false;

// SOUND ON / SOUND OFF label pointer. Assigned by buildScene() once
// the scene tree is loaded; the mouse handler toggles its text on
// every SOUND button click. nullptr in tests (no scene built) and
// during the pre-init window.
Label *g_muteLabel = nullptr;

// Processes-tab header row pointer. Assigned by buildScene(); the
// header-click path updates its cells through updateHeaderSortGlyph()
// to repaint the active sort column with a direction glyph.
Row *g_headerRow = nullptr;

// Atomic the main loop polls. Set true by SIGINT/SIGTERM (signal
// handler in coremetrics.cpp), by the --duration timer, and by the
// EXIT button click path in this TU. Lives here so the mouse handler
// can set it without coremetrics.cpp's static-qualified copy
// shadowing the symbol at link time.
std::atomic<bool> g_shutdownRequested{false};

// Live-UI state owned by KeyboardEvents.cpp (slice 13). The mouse
// handler reads + writes these from the row-select / scroll paths
// so they need the same extern visibility here.
extern int g_selectedPid;
extern int g_selectedRowIndex;
extern std::vector<int> g_visiblePids;
extern std::size_t g_processScrollOffset;
extern std::size_t g_processVisibleCount;
extern bool g_signalMenuVisible;
extern int g_signalMenuPickedIndex;

// Helper defined in src/KeyboardEvents.cpp (slice 13). The mouse
// handler gates row-selection + wheel scrolling on the Processes
// tab being active.
bool processesTabActive();
// Same TU: closeSignalMenu() resets the menu visibility flags.
void closeSignalMenu();

void activateTab(const std::string &name)
{
    Tree<Layout> &root = LayoutManager::getInstance().getRoot();
    const char *all[] = {"system", "processes", "about"};
    for (const char *n : all)
    {
        Tree<Layout> *node = EventManager::findLayoutByName(root, n);
        if (node != nullptr)
        {
            node->getData().setActive(std::string(n) == name);
        }
    }
}

// Recenter the SOUND ON / SOUND OFF label inside the mute toggle
// button. The hand-tuned baseline in base.xml is centered for the
// default "SOUND ON" string at 20 pt Body; toggling to "SOUND OFF"
// adds one glyph and would otherwise leave the label visibly
// off-center. Measures the current label text and centers it inside
// g_muteBtnMin..g_muteBtnMax. No-op if the label or TTF face is not
// ready (headless / pre-init paths). Also called from buildScene()
// in coremetrics.cpp once g_muteLabel is wired up.
void recenterMuteLabel()
{
    if (g_muteLabel == nullptr)
    {
        return;
    }
    int textWidth = Font::measureTextWidth(g_muteLabel->getText());
    if (textWidth <= 0)
    {
        return;
    }
    int btnWidth = g_muteBtnMax.x - g_muteBtnMin.x;
    int newX = g_muteBtnMin.x + (btnWidth - textWidth) / 2;
    // Keep the existing y baseline; only the x needs to track text
    // width.
    ivec2 pos = g_muteLabel->getPos();
    g_muteLabel->setPos(ivec2(newX, pos.y));
}

// Repaint the Processes-tab header row so the active sort column
// carries a trailing direction glyph (' ^' ascending, ' v'
// descending) and the other four columns stay clean. Called on
// initial scene build and on every header click so the indicator is
// sticky across polls, not only visible immediately after the user
// clicked.
void updateHeaderSortGlyph()
{
    if (g_headerRow == nullptr)
    {
        return;
    }
    const char *arrow = g_sortAscending ? " ^" : " v";
    std::vector<std::string> hdr = {"PID", "NAME", "CPU%", "MEM%", "DISK I/O"};
    hdr[static_cast<int>(g_sortColumn)] += arrow;
    g_headerRow->setCells(hdr);
}

namespace MouseEvents
{
    namespace
    {
        // Mirrors of the geometry constants coremetrics.cpp defines
        // at file scope. Keeping them as named constants here rather
        // than reaching into the other TU's `constexpr`s; if any of
        // them ever changes both sites need to update in lockstep,
        // the same lockstep slice 13 already tolerates for
        // kProcessesVisibleRows. The wider C-pillar slice that
        // unifies the Processes-table surface will fold these into
        // a shared header.
        constexpr int kResX = 960;
        constexpr int kResY = 540;
        constexpr int kProcessRowHeight = 20;
        constexpr int kProcessesFirstRowY = 100;
        constexpr int kProcessesRowX0 = 24;
        constexpr int kProcessesRowX1 = 936;
        constexpr int kProcessesVisibleRows = 15;

        // How many rows one mouse-wheel tick scrolls the Processes
        // table. Matches the in-place value the previous in-line
        // block used so wheel feel is unchanged.
        constexpr std::size_t kWheelRowsPerTick = 3;

        // Five sortable header columns: PID, NAME, CPU%, MEM%,
        // DISK I/O. The hit-test loop iterates the cached rects.
        constexpr int kHeaderColumnCount = 5;
    }

    bool handleButtonDown(SDL_MouseButtonEvent const &event)
    {
        // The window the click was delivered to; SDL_GetWindowSize
        // needs it for the letterbox math below. Looked up through
        // windowID so this TU does not depend on the main loop's
        // local handle.
        SDL_Window *window = SDL_GetWindowFromID(event.windowID);

        float clickX = 0.0f;
        float clickY = 0.0f;
        SDL_GetMouseState(&clickX, &clickY);
        int winW = 0;
        int winH = 0;
        SDL_GetWindowSize(window, &winW, &winH);
        if (winW <= 0 || winH <= 0)
        {
            winW = kResX;
            winH = kResY;
        }

        float srcAspect = static_cast<float>(kResX) / static_cast<float>(kResY);
        float dstAspect = static_cast<float>(winW) / static_cast<float>(winH);
        int viewW = winW;
        int viewH = winH;
        int viewX = 0;
        int viewY = 0;
        if (dstAspect > srcAspect)
        {
            viewH = winH;
            viewW = static_cast<int>(static_cast<float>(winH) * srcAspect);
            viewX = (winW - viewW) / 2;
        }
        else
        {
            viewW = winW;
            viewH = static_cast<int>(static_cast<float>(winW) / srcAspect);
            viewY = (winH - viewH) / 2;
        }

        float relX = clickX - static_cast<float>(viewX);
        float relY = clickY - static_cast<float>(viewY);
        if (viewW <= 0 || viewH <= 0)
        {
            return false;
        }
        int mx = static_cast<int>(relX * static_cast<float>(kResX) / static_cast<float>(viewW));
        int my = static_cast<int>(relY * static_cast<float>(kResY) / static_cast<float>(viewH));
        if (mx < 0 || my < 0 || mx >= kResX || my >= kResY)
        {
            return false;
        }

        if (mx >= g_exitBtnMin.x && mx <= g_exitBtnMax.x
            && my >= g_exitBtnMin.y && my <= g_exitBtnMax.y)
        {
            // EXIT button. The main loop polls g_shutdownRequested
            // every iteration and breaks when true; reusing the
            // shutdown atomic the SIGINT handler already sets keeps
            // the wind-down path single-sourced.
            g_shutdownRequested.store(true);
            return true;
        }
        if (mx >= g_muteBtnMin.x && mx <= g_muteBtnMax.x
            && my >= g_muteBtnMin.y && my <= g_muteBtnMax.y)
        {
            g_alarmEnabled = !g_alarmEnabled;
            if (g_muteLabel != nullptr)
            {
                g_muteLabel->setText(g_alarmEnabled ? "SOUND ON" : "SOUND OFF");
                // The toggled string is wider/narrower than the
                // previous one; recenter so the label keeps an
                // even margin on both sides of the button rect.
                recenterMuteLabel();
            }
            return true;
        }
        if (mx >= g_systemTabBtnMin.x && mx <= g_systemTabBtnMax.x
            && my >= g_systemTabBtnMin.y && my <= g_systemTabBtnMax.y)
        {
            // Intercept tab-button clicks here so the three-tab fan
            // out (hide the two non-clicked layouts, show the
            // clicked one) is one atomic switch. The Button XML
            // schema only supports one hide per button.
            activateTab("system");
            return true;
        }
        if (mx >= g_processesTabBtnMin.x && mx <= g_processesTabBtnMax.x
            && my >= g_processesTabBtnMin.y && my <= g_processesTabBtnMax.y)
        {
            activateTab("processes");
            return true;
        }
        if (mx >= g_aboutTabBtnMin.x && mx <= g_aboutTabBtnMax.x
            && my >= g_aboutTabBtnMin.y && my <= g_aboutTabBtnMax.y)
        {
            activateTab("about");
            return true;
        }

        bool headerHit = false;
        for (int c = 0; c < kHeaderColumnCount; ++c)
        {
            if (mx >= g_headerColMin[c].x && mx <= g_headerColMax[c].x
                && my >= g_headerColMin[c].y && my <= g_headerColMax[c].y)
            {
                SortColumn clicked = static_cast<SortColumn>(c);
                if (g_sortColumn == clicked)
                {
                    g_sortAscending = !g_sortAscending;
                }
                else
                {
                    g_sortColumn = clicked;
                    g_sortAscending = false;
                }
                updateHeaderSortGlyph();
                headerHit = true;
                break;
            }
        }
        if (headerHit)
        {
            return true;
        }

        // If the click landed on a Processes data row, take
        // the click as a row selection instead of letting
        // the EventManager route it to the layout tree. The
        // row widget itself does not own click handling, so
        // there is nothing else competing for this region.
        bool consumedAsRowSelect = false;
        if (processesTabActive()
            && mx >= kProcessesRowX0 && mx <= kProcessesRowX1
            && my >= kProcessesFirstRowY
            && my < kProcessesFirstRowY + kProcessesVisibleRows * kProcessRowHeight)
        {
            int row = (my - kProcessesFirstRowY) / kProcessRowHeight;
            if (row >= 0 && row < static_cast<int>(g_visiblePids.size()))
            {
                g_selectedRowIndex = row;
                g_selectedPid = g_visiblePids[row];
                consumedAsRowSelect = true;
            }
        }
        if (!consumedAsRowSelect)
        {
            EventManager::getInstance().pushEvent(std::make_unique<ClickEvent>(mx, my));
        }
        return true;
    }

    bool handleWheel(SDL_MouseWheelEvent const &event)
    {
        // Mouse-wheel scroll on the Processes tab moves the
        // visible window through the process list. y is the
        // vertical wheel delta (positive = scroll up). Each
        // wheel tick shifts by kWheelRowsPerTick rows so a
        // comfortable wrist motion covers most of the window.
        if (!processesTabActive())
        {
            return false;
        }

        constexpr std::size_t window = static_cast<std::size_t>(kProcessesVisibleRows);
        int delta = static_cast<int>(event.y);
        if (delta > 0)
        {
            std::size_t step = static_cast<std::size_t>(delta) * kWheelRowsPerTick;
            if (g_processScrollOffset >= step)
            {
                g_processScrollOffset -= step;
            }
            else
            {
                g_processScrollOffset = 0;
            }
        }
        else if (delta < 0)
        {
            std::size_t step = static_cast<std::size_t>(-delta) * kWheelRowsPerTick;
            if (g_processVisibleCount > window)
            {
                std::size_t maxOffset = g_processVisibleCount - window;
                g_processScrollOffset += step;
                if (g_processScrollOffset > maxOffset)
                {
                    g_processScrollOffset = maxOffset;
                }
            }
        }
        return true;
    }
}
