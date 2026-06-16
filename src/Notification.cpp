#include "Notification.hpp"

#include <cstdlib>
#include <string>

namespace
{
    // Wraps `in` for safe inclusion inside a single-quoted POSIX shell
    // argument. The only character that needs special handling inside
    // '...' is the single quote itself: we close the quote, emit an
    // escaped quote, then reopen. Everything else (backslashes,
    // double quotes, $, !, etc.) is literal inside single quotes.
    std::string shellEscapeForSingleQuote(const std::string &in)
    {
        std::string out;
        out.reserve(in.size() + 16);
        for (char c : in)
        {
            if (c == '\'')
            {
                out += "'\\''";
            }
            else
            {
                out += c;
            }
        }
        return out;
    }

    // osascript's `display notification` takes AppleScript string
    // literals, which are double-quoted. Inside those, backslash and
    // double quote are the two characters that need to be escaped.
    // Everything else is literal. We then wrap the whole osascript
    // argument in single quotes (via the helper above) to keep the
    // shell from re-interpreting anything.
    std::string appleScriptStringEscape(const std::string &in)
    {
        std::string out;
        out.reserve(in.size() + 16);
        for (char c : in)
        {
            if (c == '\\' || c == '"')
            {
                out += '\\';
            }
            out += c;
        }
        return out;
    }

    bool isHeadless()
    {
        const char *driver = std::getenv("SDL_VIDEO_DRIVER");
        if (driver != nullptr && std::string(driver) == "dummy")
        {
            return true;
        }
        return false;
    }
}

namespace Notification
{
    bool post(const std::string &title, const std::string &message)
    {
        if (isHeadless())
        {
            return false;
        }

#if defined(__APPLE__)
        // Build: osascript -e 'display notification "MSG" with title "TITLE"'
        // The inner double-quoted strings are AppleScript literals; the
        // outer single quotes are the shell. Both layers are escaped.
        std::string asTitle = appleScriptStringEscape(title);
        std::string asMessage = appleScriptStringEscape(message);

        std::string script;
        script.reserve(asTitle.size() + asMessage.size() + 64);
        script += "display notification \"";
        script += asMessage;
        script += "\" with title \"";
        script += asTitle;
        script += "\"";

        std::string command = "osascript -e '";
        command += shellEscapeForSingleQuote(script);
        command += "' >/dev/null 2>&1";

        return std::system(command.c_str()) == 0;

#elif defined(__linux__)
        // notify-send treats its first positional argument as the
        // summary (title) and the second as the body. A missing DISPLAY
        // means no graphical session, so we skip without invoking.
        const char *display = std::getenv("DISPLAY");
        const char *wayland = std::getenv("WAYLAND_DISPLAY");
        if ((display == nullptr || display[0] == '\0') &&
            (wayland == nullptr || wayland[0] == '\0'))
        {
            return false;
        }

        std::string command = "notify-send '";
        command += shellEscapeForSingleQuote(title);
        command += "' '";
        command += shellEscapeForSingleQuote(message);
        command += "' >/dev/null 2>&1";

        return std::system(command.c_str()) == 0;

#else
        // Windows / other: silent no-op. BurntToast would work but it
        // requires a PowerShell module install we are not willing to
        // assume. Suppress unused-parameter warnings cleanly.
        (void)title;
        (void)message;
        return false;
#endif
    }
}
