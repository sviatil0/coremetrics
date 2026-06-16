#ifndef __KILL_LOG_HPP__
#define __KILL_LOG_HPP__

#include <chrono>
#include <ctime>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>

// Append-only audit log for successful process kills performed from the
// Processes-tab signal menu. Every line is one kill event so a user can
// look back later and reconstruct exactly which pid/signal pair was sent
// at which UTC instant.
//
// Path:
//   POSIX:    $HOME/.config/coremetrics/kill.log
//   Windows:  %APPDATA%\coremetrics\kill.log
//
// Line format (one per call, newline-terminated):
//   2026-06-15T18:42:03Z TERM 12345 firefox
//
// The implementation silently swallows every I/O error (missing env var,
// disk full, permission denied, ...) because the live UI must keep
// running even if the audit trail cannot be written. The signal still
// went out either way; the audit line is a best-effort sidecar.
namespace KillLog
{
    void append(int pid,
                const std::string &name,
                const std::string &signalName);

    // Test-only overload: writes the audit line into `dir / "kill.log"`
    // rather than the platform per-user config dir. Production code paths
    // never call this; the unit-test suite does so it can round-trip
    // through a temp directory without touching the real user log. The
    // implementation is header-inline to avoid linking-order churn and to
    // keep the production translation unit (`src/KillLog.cpp`) untouched
    // by this audit-trail testability hook. Mirrors the formatting rules
    // from `src/KillLog.cpp` (timestamp + signal + pid + sanitized name)
    // so a successful round-trip here pins the same on-disk contract.
    inline void appendToDir(const std::filesystem::path &dir,
                            int pid,
                            const std::string &name,
                            const std::string &signalName)
    {
        if (dir.empty())
        {
            return;
        }

        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec)
        {
            return;
        }

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tmBuf{};
#ifdef _WIN32
        if (::gmtime_s(&tmBuf, &t) != 0)
        {
            return;
        }
#else
        if (::gmtime_r(&t, &tmBuf) == nullptr)
        {
            return;
        }
#endif
        char timeBuf[32];
        std::size_t written = std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%SZ", &tmBuf);
        if (written == 0)
        {
            return;
        }

        // Flatten whitespace + collapse empty fields to '-'. Duplicated
        // from src/KillLog.cpp on purpose so the on-disk contract is
        // single-sourced from this header for the test overload while
        // production code keeps using the private helpers in the .cpp.
        std::string signalOut;
        signalOut.reserve(signalName.size());
        for (char c : signalName)
        {
            if (c == '\n' || c == '\r' || c == '\t')
            {
                signalOut.push_back(' ');
            }
            else
            {
                signalOut.push_back(c);
            }
        }
        if (signalOut.empty())
        {
            signalOut = "-";
        }

        // Strip tree-decoration prefix the Processes tab adds in tree
        // mode (spaces, tabs, '|', '-', '+') so the audit line carries
        // the bare executable name even in tree view.
        std::size_t begin = 0;
        while (begin < name.size())
        {
            char c = name[begin];
            if (c != ' ' && c != '\t' && c != '|' && c != '-' && c != '+')
            {
                break;
            }
            ++begin;
        }
        std::string nameOut;
        nameOut.reserve(name.size() - begin);
        for (std::size_t i = begin; i < name.size(); ++i)
        {
            char c = name[i];
            if (c == '\n' || c == '\r' || c == '\t')
            {
                nameOut.push_back(' ');
            }
            else
            {
                nameOut.push_back(c);
            }
        }
        if (nameOut.empty())
        {
            nameOut = "-";
        }

        std::filesystem::path logPath = dir / "kill.log";
        std::ofstream out(logPath, std::ios::out | std::ios::app);
        if (!out.is_open())
        {
            return;
        }

        out << std::string(timeBuf, written) << ' '
            << signalOut << ' '
            << pid << ' '
            << nameOut << '\n';
    }
}

#endif
