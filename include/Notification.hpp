#ifndef __NOTIFICATION_HPP__
#define __NOTIFICATION_HPP__

#include <string>

// Per-platform desktop notification backend. Shells out to the OS-native
// notifier so we do not need a new library dependency or a per-platform
// SDK:
//   - macOS:  osascript -e 'display notification "..." with title "..."'
//   - Linux:  notify-send '...' '...'
//   - Windows: silent no-op (BurntToast needs a PowerShell module install,
//     which is too much friction for a TUI-adjacent app)
//
// Headless / unsupported sessions return false without invoking anything:
//   - SDL_VIDEO_DRIVER == "dummy"      (CI / screenshot mode on any OS)
//   - DISPLAY unset                    (headless Linux)
//
// SAFETY: all user-controlled strings are routed through a single-quote
// shell escape before being embedded into the command line. Never call
// system() with raw inputs.
namespace Notification
{
    // Returns true when the platform notifier was actually invoked (i.e.
    // system() returned 0). Returns false on every other path: headless,
    // unsupported platform, and notifier failure. The return value is
    // advisory; the caller should treat the notification as best-effort.
    bool post(const std::string &title, const std::string &message);
}

#endif
