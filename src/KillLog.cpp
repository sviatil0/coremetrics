#include "KillLog.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace
{
    // Directory name reused on every platform under the per-user config
    // root. Same spelling as Settings.cpp so the audit log lands next to
    // the existing config file rather than in a sibling directory.
    const std::string CONFIG_DIR_NAME = "coremetrics";
    const std::string KILL_LOG_FILE_NAME = "kill.log";

    // Resolve the per-user CoreMetrics config directory. Returns an empty
    // path when the relevant env var is missing so the caller can bail
    // out without ever touching the filesystem. Mirrors Settings.cpp's
    // resolver so both files share the exact same parent directory.
    std::filesystem::path resolveConfigDir()
    {
#ifdef _WIN32
        const char *appdata = std::getenv("APPDATA");
        if (appdata == nullptr || appdata[0] == '\0')
        {
            return std::filesystem::path{};
        }
        return std::filesystem::path(appdata) / CONFIG_DIR_NAME;
#else
        const char *home = std::getenv("HOME");
        if (home == nullptr || home[0] == '\0')
        {
            return std::filesystem::path{};
        }
        return std::filesystem::path(home) / ".config" / CONFIG_DIR_NAME;
#endif
    }

    // Format the current wall-clock time as ISO 8601 in UTC with a Z
    // suffix. gmtime_r / gmtime_s is portable across the three target
    // platforms and avoids pulling std::format formatters for chrono,
    // which still produce locale-dependent output on some libc++ builds.
    // Returns an empty string when the system clock conversion fails so
    // the caller can skip the write and avoid a half-formed line.
    std::string isoTimestampUtc()
    {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tmBuf{};
#ifdef _WIN32
        if (::gmtime_s(&tmBuf, &t) != 0)
        {
            return std::string{};
        }
#else
        if (::gmtime_r(&t, &tmBuf) == nullptr)
        {
            return std::string{};
        }
#endif
        char buf[32];
        std::size_t written = std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmBuf);
        if (written == 0)
        {
            return std::string{};
        }
        return std::string(buf, written);
    }

    // True for the leading indent / connector glyphs the Processes tab
    // prepends to a name cell in tree mode ("  |- firefox", "+ chrome").
    // Used to skip past that decoration so the audit line carries the
    // bare process name rather than a half-readable tree fragment.
    bool isTreeIndentChar(char c)
    {
        return c == ' ' || c == '\t' || c == '|' || c == '-' || c == '+';
    }

    // Coerce a field to single-line, space-safe text. Newlines / CRs /
    // tabs become spaces so a later `cat` / log parser does not
    // misinterpret the line as multiple records. Returns a copy so the
    // caller's original string is never mutated. An empty input becomes
    // a single `-` so the column is never missing from a line.
    std::string flattenWhitespace(const std::string &raw)
    {
        std::string out;
        out.reserve(raw.size());
        for (char c : raw)
        {
            if (c == '\n' || c == '\r' || c == '\t')
            {
                out.push_back(' ');
            }
            else
            {
                out.push_back(c);
            }
        }
        if (out.empty())
        {
            out = "-";
        }
        return out;
    }

    // Same as flattenWhitespace but additionally strips the leading
    // tree-indent decoration the Processes tab adds in tree mode. Used
    // for the process-name field so the audit line records the bare
    // executable name even when the user is browsing the process tree.
    std::string sanitizeProcessName(const std::string &raw)
    {
        std::size_t begin = 0;
        while (begin < raw.size() && isTreeIndentChar(raw[begin]))
        {
            ++begin;
        }
        return flattenWhitespace(raw.substr(begin));
    }
}

namespace KillLog
{

void append(int pid, const std::string &name, const std::string &signalName)
{
    std::filesystem::path dir = resolveConfigDir();
    if (dir.empty())
    {
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec)
    {
        // Best-effort: a missing parent directory we cannot create is a
        // hard stop for this kill event, but the live UI must keep going.
        return;
    }

    std::string timestamp = isoTimestampUtc();
    if (timestamp.empty())
    {
        return;
    }

    std::filesystem::path logPath = dir / KILL_LOG_FILE_NAME;
    std::ofstream out(logPath, std::ios::out | std::ios::app);
    if (!out.is_open())
    {
        return;
    }

    out << timestamp << ' '
        << flattenWhitespace(signalName) << ' '
        << pid << ' '
        << sanitizeProcessName(name) << '\n';
    // ofstream destructor flushes; explicit close is unnecessary. Any
    // disk-full / quota error surfaces as a failbit which we ignore by
    // design so a write failure does not crash the UI thread.
}

}
