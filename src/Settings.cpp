#include "Settings.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

// Persistence for the four runtime-tweakable knobs CoreMetrics tracks
// (poll cadence, sparklines toggle, sort column, sort direction). The
// file format is a flat list of `key=value` lines, no escaping, no
// nesting, so it can be hand-edited and is also trivial to diff.
//
// Path resolution mirrors the platform convention without pulling in a
// dependency: $HOME/.config/coremetrics/config on POSIX, and
// %APPDATA%/coremetrics/config on Windows. If neither env var is set
// (very rare; sandboxed CI sometimes strips them) we bail rather than
// fall back to the working directory, which could leak prefs into a
// random checkout.

namespace
{
    // Single file name lives under the per-platform config directory.
    // Bare basename so the parent path can be reused for any future
    // sidecar files without changing the lookup logic.
    const std::string CONFIG_DIR_NAME = "coremetrics";
    const std::string CONFIG_FILE_NAME = "config";

    // Recognized key names. Keeping these as named constants makes the
    // read and write paths share the exact same spelling, and any future
    // rename is a single-line edit.
    const std::string KEY_POLL_MS = "poll_ms";
    const std::string KEY_SPARKLINES = "sparklines_enabled";
    const std::string KEY_SORT_COLUMN = "sort_column";
    const std::string KEY_SORT_ASCENDING = "sort_ascending";
    const std::string KEY_COLLAPSED_PIDS = "collapsed_pids";

    // Pid values come from kernel APIs that always return non-negative
    // integers, so the parsed list rejects anything outside this range.
    // Cap at INT32_MAX so a corrupt file with a giant number cannot
    // overflow the int the set is keyed on.
    constexpr int PID_MIN = 0;
    constexpr int PID_MAX = 2147483647;

    // Lower and upper bounds on the persisted poll cadence. These match
    // the clamp the CLI parser applies, so a hand-edited config file
    // cannot smuggle in a value the live UI would otherwise reject.
    constexpr std::uint64_t POLL_MS_MIN = 100;
    constexpr std::uint64_t POLL_MS_MAX = 10000;

    // SortColumn enum has values 0..4 (PID/NAME/CPU/MEM/DISK). The
    // bounds are checked against this range when loading so a corrupt
    // file cannot push the in-memory enum out of range.
    constexpr int SORT_COLUMN_MIN = 0;
    constexpr int SORT_COLUMN_MAX = 4;

    // Resolve the per-user CoreMetrics config directory. Returns an
    // empty path on platforms where the relevant env var is missing,
    // which the caller treats as "no config available" rather than as
    // an error.
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

    std::filesystem::path resolveConfigPath()
    {
        std::filesystem::path dir = resolveConfigDir();
        if (dir.empty())
        {
            return std::filesystem::path{};
        }
        return dir / CONFIG_FILE_NAME;
    }

    // Trim ASCII whitespace from both ends of a string in place. The
    // config parser uses it on the key and value so a hand-edited file
    // with stray spaces around the `=` still parses cleanly.
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

    // Best-effort parse of a non-negative integer. Returns true and
    // fills `out` on success; returns false on empty input, on any
    // non-digit character, or on overflow. The parser deliberately
    // rejects negative numbers because none of the persisted values
    // can legitimately be negative.
    bool parseUnsigned(const std::string &raw, std::uint64_t &out)
    {
        if (raw.empty())
        {
            return false;
        }
        std::uint64_t value = 0;
        for (char c : raw)
        {
            if (c < '0' || c > '9')
            {
                return false;
            }
            std::uint64_t digit = static_cast<std::uint64_t>(c - '0');
            if (value > (UINT64_MAX - digit) / 10)
            {
                return false;
            }
            value = value * 10 + digit;
        }
        out = value;
        return true;
    }

    // Split the comma-separated `collapsed_pids` value and insert each
    // valid pid into the destination set. Invalid tokens (empty, non-
    // digit, out-of-range) are silently skipped so a partially corrupt
    // line does not prevent the rest of the pids from being restored.
    // The destination is appended to rather than overwritten so a
    // hypothetical future split into multiple keys would compose.
    void parseCollapsedPids(const std::string &raw,
                            std::unordered_set<int> &out)
    {
        std::size_t cursor = 0;
        while (cursor <= raw.size())
        {
            std::size_t comma = raw.find(',', cursor);
            std::size_t end = (comma == std::string::npos)
                                  ? raw.size()
                                  : comma;
            std::string token = raw.substr(cursor, end - cursor);
            trimInPlace(token);
            if (!token.empty())
            {
                std::uint64_t parsed = 0;
                if (parseUnsigned(token, parsed)
                    && parsed <= static_cast<std::uint64_t>(PID_MAX))
                {
                    int pid = static_cast<int>(parsed);
                    if (pid >= PID_MIN)
                    {
                        out.insert(pid);
                    }
                }
            }
            if (comma == std::string::npos)
            {
                break;
            }
            cursor = comma + 1;
        }
    }

    // Accept both word-form and digit-form booleans so a hand-edited
    // file with `true`/`false` round-trips and so the writer can keep
    // emitting `0`/`1` for the smallest possible footprint.
    bool parseBool(const std::string &raw, bool &out)
    {
        if (raw == "1" || raw == "true" || raw == "TRUE" || raw == "True")
        {
            out = true;
            return true;
        }
        if (raw == "0" || raw == "false" || raw == "FALSE" || raw == "False")
        {
            out = false;
            return true;
        }
        return false;
    }
}

bool Settings::load(std::uint64_t &pollIntervalMs,
                    bool &sparklinesEnabled,
                    int &sortColumn,
                    bool &sortAscending,
                    std::unordered_set<int> &collapsedPids)
{
    std::filesystem::path configPath = resolveConfigPath();
    if (configPath.empty())
    {
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(configPath, ec) || ec)
    {
        return false;
    }

    std::ifstream in(configPath);
    if (!in.is_open())
    {
        return false;
    }

    std::string line;
    while (std::getline(in, line))
    {
        // Skip blank lines and `#`-prefixed comments so a hand-edited
        // file can carry inline notes without breaking the loader.
        std::string trimmed = line;
        trimInPlace(trimmed);
        if (trimmed.empty() || trimmed[0] == '#')
        {
            continue;
        }

        // A valid record has exactly one `=`. Anything else (no `=`,
        // empty key, empty value) is silently skipped rather than
        // aborting the whole file: best-effort wins over strict.
        std::size_t eq = trimmed.find('=');
        if (eq == std::string::npos)
        {
            continue;
        }
        std::string key = trimmed.substr(0, eq);
        std::string value = trimmed.substr(eq + 1);
        trimInPlace(key);
        trimInPlace(value);
        if (key.empty() || value.empty())
        {
            continue;
        }

        if (key == KEY_POLL_MS)
        {
            std::uint64_t parsed = 0;
            if (parseUnsigned(value, parsed))
            {
                if (parsed < POLL_MS_MIN)
                {
                    parsed = POLL_MS_MIN;
                }
                if (parsed > POLL_MS_MAX)
                {
                    parsed = POLL_MS_MAX;
                }
                pollIntervalMs = parsed;
            }
        }
        else if (key == KEY_SPARKLINES)
        {
            bool parsed = false;
            if (parseBool(value, parsed))
            {
                sparklinesEnabled = parsed;
            }
        }
        else if (key == KEY_SORT_COLUMN)
        {
            std::uint64_t parsed = 0;
            if (parseUnsigned(value, parsed))
            {
                int asInt = static_cast<int>(parsed);
                if (asInt >= SORT_COLUMN_MIN && asInt <= SORT_COLUMN_MAX)
                {
                    sortColumn = asInt;
                }
            }
        }
        else if (key == KEY_SORT_ASCENDING)
        {
            bool parsed = false;
            if (parseBool(value, parsed))
            {
                sortAscending = parsed;
            }
        }
        else if (key == KEY_COLLAPSED_PIDS)
        {
            // Replace whatever the caller seeded the set with: if the
            // file says "these are the collapsed pids", we honour it
            // even when the seed was non-empty. Stale pids from prior
            // runs are pruned by the renderer the moment those pids
            // are no longer alive, so persisting them carries no cost.
            collapsedPids.clear();
            parseCollapsedPids(value, collapsedPids);
        }
        // Unknown keys are ignored so future versions can drop or
        // rename keys without making old configs unreadable.
    }

    return true;
}

bool Settings::save(std::uint64_t pollIntervalMs,
                    bool sparklinesEnabled,
                    int sortColumn,
                    bool sortAscending,
                    const std::unordered_set<int> &collapsedPids)
{
    std::filesystem::path configPath = resolveConfigPath();
    if (configPath.empty())
    {
        return false;
    }

    std::filesystem::path configDir = configPath.parent_path();
    if (!configDir.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(configDir, ec);
        if (ec)
        {
            return false;
        }
    }

    std::ofstream out(configPath, std::ios::out | std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    // Header comment so a curious user opening the file sees what it
    // is and how the loader treats unknown keys.
    out << "# CoreMetrics persisted runtime settings.\n";
    out << "# Edit by hand at your own risk; malformed lines are skipped.\n";

    std::ostringstream body;
    body << KEY_POLL_MS << '=' << pollIntervalMs << '\n';
    body << KEY_SPARKLINES << '=' << (sparklinesEnabled ? 1 : 0) << '\n';
    body << KEY_SORT_COLUMN << '=' << sortColumn << '\n';
    body << KEY_SORT_ASCENDING << '=' << (sortAscending ? 1 : 0) << '\n';

    // Emit the collapsed-pids line only when the set is non-empty so a
    // user who never touches tree mode keeps a clean four-line config.
    // Sort the pids before writing so the on-disk format is stable
    // across runs (unordered_set iteration order is implementation
    // defined) and so diffing two configs is meaningful.
    if (!collapsedPids.empty())
    {
        std::vector<int> sorted(collapsedPids.begin(), collapsedPids.end());
        std::sort(sorted.begin(), sorted.end());
        body << KEY_COLLAPSED_PIDS << '=';
        for (std::size_t i = 0; i < sorted.size(); ++i)
        {
            if (i > 0)
            {
                body << ',';
            }
            body << sorted[i];
        }
        body << '\n';
    }
    out << body.str();

    return out.good();
}
