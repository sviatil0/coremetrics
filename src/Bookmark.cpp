#include "Bookmark.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

// Persistence for the "starred" / bookmarked processes list. Format,
// path layout and error policy are documented in Bookmark.hpp.

namespace
{
    // Directory and file names. The directory matches the one
    // Settings.cpp and KillLog.cpp resolve to so all CoreMetrics sidecar
    // files land in the same per-user config tree.
    const std::string CONFIG_DIR_NAME = "coremetrics";
    const std::string BOOKMARK_FILE_NAME = "bookmarks";

    // Resolve the per-user CoreMetrics config directory. Mirrors the
    // resolver in Settings.cpp / KillLog.cpp so every CoreMetrics file
    // shares the exact same parent path. Returns an empty path when the
    // relevant env var is missing so the caller can bail without ever
    // touching the filesystem.
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

    // Trim ASCII whitespace (space, tab, CR, LF) from both ends of a
    // string in place. Used so a hand-edited file with stray spaces
    // around a name or pid still loads cleanly.
    void trimInPlace(std::string &s)
    {
        std::size_t begin = 0;
        while (begin < s.size()
               && (s[begin] == ' ' || s[begin] == '\t'
                   || s[begin] == '\r' || s[begin] == '\n'))
        {
            ++begin;
        }
        std::size_t end = s.size();
        while (end > begin
               && (s[end - 1] == ' ' || s[end - 1] == '\t'
                   || s[end - 1] == '\r' || s[end - 1] == '\n'))
        {
            --end;
        }
        s = s.substr(begin, end - begin);
    }

    // True when every character after the first non-empty position is
    // an ASCII digit. Empty string returns false. Used to validate the
    // pid half of a `name|pid` record.
    bool isAllDigits(const std::string &s)
    {
        if (s.empty())
        {
            return false;
        }
        for (char c : s)
        {
            if (c < '0' || c > '9')
            {
                return false;
            }
        }
        return true;
    }

    // Validate one raw line read from the bookmarks file. Returns the
    // canonical in-memory form (verbatim line) on success and an empty
    // string when the line should be skipped. Accepted shapes:
    //   "name"        -> wildcard form, kept as-is
    //   "name|pid"    -> exact form, kept as-is when pid is all digits
    // Rejected shapes (silently skipped, never throw):
    //   ""            empty line
    //   "|123"        leading pipe / empty name
    //   "name|"       trailing pipe / empty pid
    //   "name|abc"    non-digit pid
    //   "a|b|c"       more than one pipe
    std::string sanitizeRecord(const std::string &raw)
    {
        std::string trimmed = raw;
        trimInPlace(trimmed);
        if (trimmed.empty())
        {
            return std::string{};
        }

        std::size_t firstPipe = trimmed.find('|');
        if (firstPipe == std::string::npos)
        {
            // Pure wildcard form. Re-trim individual side here would be
            // a no-op since we already trimmed the whole line.
            return trimmed;
        }

        // Reject extra pipes so a corrupt write cannot smuggle in a
        // record that round-trips into something the loader cannot
        // round-trip again.
        if (trimmed.find('|', firstPipe + 1) != std::string::npos)
        {
            return std::string{};
        }

        std::string name = trimmed.substr(0, firstPipe);
        std::string pid = trimmed.substr(firstPipe + 1);
        trimInPlace(name);
        trimInPlace(pid);
        if (name.empty() || pid.empty() || !isAllDigits(pid))
        {
            return std::string{};
        }
        return name + "|" + pid;
    }
}

namespace Bookmark
{

bool load(const std::string &path, std::unordered_set<std::string> &out)
{
    out.clear();

    if (path.empty())
    {
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec)
    {
        // Missing file is not an error in the failure-mode sense, but
        // we still surface it as `false` so the caller can distinguish
        // "no file yet" from "loaded zero records from an existing
        // file". The destination set has been cleared above either way.
        return false;
    }

    std::ifstream in(path);
    if (!in.is_open())
    {
        return false;
    }

    std::string line;
    while (std::getline(in, line))
    {
        std::string canonical = sanitizeRecord(line);
        if (!canonical.empty())
        {
            out.insert(canonical);
        }
        // Malformed lines are silently dropped: best-effort wins over
        // strict so one corrupt record does not blow away the rest.
    }

    return true;
}

bool save(const std::string &path, const std::unordered_set<std::string> &in)
{
    if (path.empty())
    {
        return false;
    }

    std::filesystem::path target(path);
    std::filesystem::path dir = target.parent_path();
    if (!dir.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec)
        {
            return false;
        }
    }

    std::ofstream out(target, std::ios::out | std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    // Sort the records before writing so the on-disk format is stable
    // across runs (unordered_set iteration order is implementation
    // defined) and so diffing two bookmark files is meaningful. An
    // empty set still truncates the file to zero bytes, which is the
    // documented signal for "user has explicitly cleared their pins".
    std::vector<std::string> sorted(in.begin(), in.end());
    std::sort(sorted.begin(), sorted.end());
    for (const std::string &record : sorted)
    {
        out << record << '\n';
    }

    return out.good();
}

std::string defaultPath()
{
    std::filesystem::path dir = resolveConfigDir();
    if (dir.empty())
    {
        return std::string{};
    }
    return (dir / BOOKMARK_FILE_NAME).string();
}

}
