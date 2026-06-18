#include "KeyboardEvents.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

#include "EventManager.hpp"
#include "KillLog.hpp"
#include "Layout.hpp"
#include "LayoutManager.hpp"
#include "Row.hpp"
#include "SignalUtils.hpp"
#include "Tree.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 13 of the
// modernization roadmap. The previous in-place block lived inside the
// SDL_PollEvent switch and dispatched every keystroke the live UI
// reacts to. It read and wrote a long list of file-scope globals plus
// a handful of file-scope helpers; this TU keeps the same data
// ownership and consults those globals via extern declarations.
//
// The globals listed below moved here from coremetrics.cpp so the
// tests binary (which links every src/*.cpp object but not the
// coremetrics.cpp TU) resolves the symbols KeyboardEvents references.
// coremetrics.cpp now declares them extern at the top of its file.
//   g_helpOverlayVisible, g_signalMenuVisible, g_signalMenuPickedIndex,
//   g_selectedPid, g_selectedRowIndex, g_filterText, g_filterInputActive,
//   g_treeMode, g_processScrollOffset, g_processVisibleCount,
//   g_visiblePids, g_processRows, g_collapsedPids.
// The three helpers processesTabActive(), closeSignalMenu(), and
// flashStatus() lost their `static` in coremetrics.cpp so we can
// call them from here without duplicating their (trivial) bodies.
// Future slices (C9 EventHandlers) will continue tightening this
// surface.

bool g_helpOverlayVisible = false;
bool g_signalMenuVisible = false;
int g_signalMenuPickedIndex = -1; // -1 = picking, 0..5 = awaiting confirm
int g_selectedPid = -1;
int g_selectedRowIndex = -1;
std::string g_filterText;
bool g_filterInputActive = false;
bool g_treeMode = false;
std::size_t g_processScrollOffset = 0;
std::size_t g_processVisibleCount = 0;
std::vector<int> g_visiblePids;
std::vector<Row *> g_processRows;
std::unordered_set<int> g_collapsedPids;

// flashStatus() backing store. Lives next to the writer; the read
// path in coremetrics.cpp's renderer consults the same symbols via
// extern declarations.
std::string g_statusFlash;
Uint64 g_statusFlashExpiryMs = 0;

namespace
{
    // How long a "sent TERM -> pid N" flash stays on screen. Mirrors
    // the value coremetrics.cpp used before the slice; kept here so
    // flashStatus() owns its own cadence.
    constexpr Uint64 kStatusFlashDurationMs = 2500;
}

void flashStatus(const std::string &text)
{
    g_statusFlash = text;
    g_statusFlashExpiryMs = SDL_GetTicks() + kStatusFlashDurationMs;
}

bool processesTabActive()
{
    Tree<Layout> *node = EventManager::findLayoutByName(
        LayoutManager::getInstance().getRoot(), "processes");
    return node != nullptr && node->getData().isActive();
}

void closeSignalMenu()
{
    g_signalMenuVisible = false;
    g_signalMenuPickedIndex = -1;
}

namespace KeyboardEvents
{
    namespace
    {
        // Mirror of coremetrics.cpp's PROCESSES_VISIBLE_ROWS. Keeping
        // this as a named constant rather than a magic number; if the
        // visible row count ever changes both sites need to update
        // in lockstep. The wider C-pillar slice that finally pulls the
        // process table out will let us unify the constant in one
        // header.
        constexpr int kProcessesVisibleRows = 15;
    }

    bool handle(SDL_KeyboardEvent const &event)
    {
        SDL_Keycode key = event.key;

        // The window the keystroke was delivered to; SDL_StartTextInput
        // / SDL_StopTextInput need it. Looked up through windowID so
        // this TU does not depend on the main loop's local handle.
        SDL_Window *window = SDL_GetWindowFromID(event.windowID);

        // '?' toggles the keyboard shortcuts overlay. Lives before
        // every other handler so it works on any tab and even with
        // another overlay underneath; the help panel is read-only
        // chrome and never consumes a follow-up keystroke besides
        // its own dismiss.
        if (key == SDLK_QUESTION)
        {
            g_helpOverlayVisible = !g_helpOverlayVisible;
            return true;
        }

        // Esc and N close the signal menu without sending; if no
        // menu is open Esc clears the selection (the htop default).
        if (key == SDLK_ESCAPE)
        {
            if (g_helpOverlayVisible)
            {
                // Esc on help overlay: dismiss only. Don't fall
                // through to clearing selection or filter so the
                // user can dismiss without side effects.
                g_helpOverlayVisible = false;
                return true;
            }
            if (g_signalMenuVisible)
            {
                closeSignalMenu();
            }
            else if (g_filterInputActive)
            {
                // Esc while editing: drop the filter entirely so
                // a typo or accidental '/' isn't sticky.
                g_filterInputActive = false;
                g_filterText.clear();
                SDL_StopTextInput(window);
            }
            else if (!g_filterText.empty())
            {
                // Esc with filter applied but not editing: clear
                // the filter, mirrors htop F3.
                g_filterText.clear();
            }
            else
            {
                g_selectedPid = -1;
                g_selectedRowIndex = -1;
            }
            return true;
        }

        // Backspace in filter input mode edits the filter; outside
        // input mode it does nothing (no other use).
        if (g_filterInputActive && key == SDLK_BACKSPACE)
        {
            if (!g_filterText.empty())
            {
                g_filterText.pop_back();
            }
            return true;
        }

        // Enter commits the filter and exits input mode; the
        // filter stays applied and Up/Down + 'k' resume working.
        if (g_filterInputActive && (key == SDLK_RETURN || key == SDLK_KP_ENTER))
        {
            g_filterInputActive = false;
            SDL_StopTextInput(window);
            return true;
        }

        // While the user is editing the filter, do NOT let other
        // bindings (Up/Down/k/1-6) fire underneath; that would be
        // surprising and the SDL_StartTextInput path may have
        // already enqueued the keystroke as text.
        if (g_filterInputActive)
        {
            return true;
        }

        if (g_signalMenuVisible)
        {
            if (g_signalMenuPickedIndex < 0)
            {
                // Picking phase: 1..6 chooses a signal, anything
                // else is ignored so a stray keystroke cannot send.
                if (key >= SDLK_1 && key <= SDLK_6)
                {
                    g_signalMenuPickedIndex = static_cast<int>(key - SDLK_1);
                }
            }
            else
            {
                // Confirm phase: Y / Enter sends, N / Esc cancels.
                if (key == SDLK_Y || key == SDLK_RETURN || key == SDLK_KP_ENTER)
                {
                    SignalUtils::Signal sig =
                        static_cast<SignalUtils::Signal>(g_signalMenuPickedIndex);
                    SignalUtils::SendStatus s = SignalUtils::send(g_selectedPid, sig);
                    std::string label = std::string(SignalUtils::name(sig))
                                         + " -> " + std::to_string(g_selectedPid);
                    switch (s)
                    {
                    case SignalUtils::SendStatus::Ok:
                        flashStatus("sent " + label);
                        // Audit trail: one line per successful kill so a
                        // user can later reconstruct which signal landed
                        // on which pid. Best-effort and silent on I/O
                        // failure so the live UI never blocks on disk.
                        KillLog::append(g_selectedPid,
                                        (g_selectedRowIndex >= 0
                                         && g_selectedRowIndex + 1 < static_cast<int>(g_processRows.size())
                                         && g_processRows[g_selectedRowIndex + 1] != nullptr
                                         && g_processRows[g_selectedRowIndex + 1]->getCells().size() > 1)
                                            ? g_processRows[g_selectedRowIndex + 1]->getCells()[1]
                                            : std::string{},
                                        SignalUtils::name(sig));
                        break;
                    case SignalUtils::SendStatus::NoSuchProcess:
                        flashStatus(label + " : no such process");
                        break;
                    case SignalUtils::SendStatus::PermissionDenied:
                        flashStatus(label + " : permission denied");
                        break;
                    case SignalUtils::SendStatus::InvalidSignal:
                        flashStatus(label + " : invalid signal");
                        break;
                    case SignalUtils::SendStatus::InvalidPid:
                        flashStatus(label + " : refused (pid <= 1)");
                        break;
                    case SignalUtils::SendStatus::Unsupported:
                        flashStatus(label + " : windows: not implemented");
                        break;
                    }
                    closeSignalMenu();
                }
                else if (key == SDLK_N)
                {
                    closeSignalMenu();
                }
            }
            return true;
        }

        // No menu open: arrow keys move row selection; 'k' opens
        // the menu when a row is selected and the Processes tab
        // is active.
        if (!processesTabActive())
        {
            return false;
        }
        if (key == SDLK_UP || key == SDLK_DOWN)
        {
            if (g_visiblePids.empty())
            {
                return true;
            }
            int next = g_selectedRowIndex;
            if (next < 0)
            {
                next = 0;
            }
            else if (key == SDLK_UP)
            {
                if (next > 0)
                {
                    next -= 1;
                }
                else if (g_processScrollOffset > 0)
                {
                    // Already at top visible row; scroll the
                    // window up by one so the previous off-screen
                    // process becomes the new selection on the
                    // next poll tick.
                    g_processScrollOffset -= 1;
                }
            }
            else
            {
                int max = static_cast<int>(g_visiblePids.size()) - 1;
                if (next < max)
                {
                    next += 1;
                }
                else if (g_processVisibleCount
                         > g_processScrollOffset + g_visiblePids.size())
                {
                    // Already at bottom visible row; scroll the
                    // window down by one so the next off-screen
                    // process becomes the new selection.
                    g_processScrollOffset += 1;
                }
            }
            g_selectedRowIndex = next;
            g_selectedPid = g_visiblePids[next];
        }
        else if (key == SDLK_PAGEUP)
        {
            if (g_processScrollOffset >= kProcessesVisibleRows)
            {
                g_processScrollOffset -= kProcessesVisibleRows;
            }
            else
            {
                g_processScrollOffset = 0;
            }
        }
        else if (key == SDLK_PAGEDOWN)
        {
            std::size_t dataRowCount = kProcessesVisibleRows;
            if (g_processVisibleCount > dataRowCount)
            {
                std::size_t maxOffset = g_processVisibleCount - dataRowCount;
                g_processScrollOffset += kProcessesVisibleRows;
                if (g_processScrollOffset > maxOffset)
                {
                    g_processScrollOffset = maxOffset;
                }
            }
        }
        else if (key == SDLK_HOME)
        {
            g_processScrollOffset = 0;
            if (!g_visiblePids.empty())
            {
                g_selectedRowIndex = 0;
                g_selectedPid = g_visiblePids.front();
            }
        }
        else if (key == SDLK_END)
        {
            std::size_t dataRowCount = kProcessesVisibleRows;
            if (g_processVisibleCount > dataRowCount)
            {
                g_processScrollOffset = g_processVisibleCount - dataRowCount;
            }
            else
            {
                g_processScrollOffset = 0;
            }
        }
        else if (key == SDLK_K && g_selectedPid >= 0)
        {
            g_signalMenuVisible = true;
            g_signalMenuPickedIndex = -1;
        }
        else if (key == SDLK_SLASH)
        {
            // '/' on Processes tab enters filter input mode. If
            // a filter is already applied, re-enter with the
            // existing text so the user can keep typing.
            g_filterInputActive = true;
            SDL_StartTextInput(window);
        }
        else if (key == SDLK_T)
        {
            // 't' toggles tree mode. When the user is editing a
            // filter the key path was already swallowed above, so
            // the toggle only fires when input is idle.
            g_treeMode = !g_treeMode;
            flashStatus(g_treeMode ? "tree mode on" : "tree mode off");
        }
        else if (key == SDLK_SPACE && g_treeMode && g_selectedPid >= 0)
        {
            // Space collapses/expands the subtree rooted at the
            // selected pid. State lives in g_collapsedPids and is
            // consulted by the tree-mode flattener; the next poll
            // hides (or restores) the descendants.
            auto it = g_collapsedPids.find(g_selectedPid);
            if (it == g_collapsedPids.end())
            {
                g_collapsedPids.insert(g_selectedPid);
                flashStatus("collapsed pid " + std::to_string(g_selectedPid));
            }
            else
            {
                g_collapsedPids.erase(it);
                flashStatus("expanded pid " + std::to_string(g_selectedPid));
            }
        }
        else
        {
            return false;
        }
        return true;
    }
}
