#include "TraceLog.hpp"

#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <system_error>

namespace
{
    // Active flag for the facility. Marked atomic so the fast disabled
    // path in log() does not have to take the mutex on every call from
    // the UI / metrics thread. enable() / disable() always set this
    // under the mutex so the flag and the stream stay consistent.
    std::atomic<bool> g_active{false};

    // Serializes writes so two threads cannot interleave bytes inside a
    // single log line. Also guards enable() / disable() so the stream
    // pointer never changes mid-write.
    std::mutex g_mutex;

    // Owning handle for the trace file. Held by pointer so enable() can
    // rebind it to a new path without leaving a half-open stream
    // dangling on the stack of a previous call.
    std::ofstream g_stream;

    // Format the current wall-clock time as ISO 8601 in UTC with a Z
    // suffix. gmtime_r / gmtime_s is portable across the three target
    // platforms and avoids pulling std::format formatters for chrono,
    // which still produce locale-dependent output on some libc++ builds.
    // Returns an empty string when the clock conversion fails so the
    // caller can skip the write rather than emit a half-formed line.
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
}

namespace TraceLog
{

void enable(const std::string &path)
{
    std::lock_guard<std::mutex> guard(g_mutex);

    // Best-effort: try to create the parent directory so a fresh
    // install does not have to mkdir the config dir by hand. Failure
    // here is not fatal; the ofstream open below will simply fail and
    // leave the facility disabled.
    std::filesystem::path filePath(path);
    std::filesystem::path parent = filePath.parent_path();
    if (!parent.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
    }

    // Drop any previous stream so a second enable() with a new path
    // does not leak the old file descriptor.
    if (g_stream.is_open())
    {
        g_stream.close();
    }
    g_stream.clear();

    g_stream.open(filePath, std::ios::out | std::ios::app);
    if (!g_stream.is_open())
    {
        g_active.store(false, std::memory_order_release);
        return;
    }

    g_active.store(true, std::memory_order_release);
}

void disable()
{
    std::lock_guard<std::mutex> guard(g_mutex);
    g_active.store(false, std::memory_order_release);
    if (g_stream.is_open())
    {
        g_stream.flush();
        g_stream.close();
    }
    g_stream.clear();
}

bool isEnabled()
{
    return g_active.load(std::memory_order_acquire);
}

void log(const std::string &message)
{
    // Fast disabled-path: a single atomic-bool load and we are out.
    // Every caller in the hot loops pays this cost only.
    if (!g_active.load(std::memory_order_acquire))
    {
        return;
    }

    std::string timestamp = isoTimestampUtc();
    if (timestamp.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> guard(g_mutex);
    // Re-check under the lock: another thread may have called disable()
    // between the fast check and the lock acquisition. Skipping this
    // would write into a closed stream and silently fail anyway, but
    // doing it explicitly keeps the contract obvious.
    if (!g_active.load(std::memory_order_acquire) || !g_stream.is_open())
    {
        return;
    }

    g_stream << timestamp << ' ' << message << '\n';
    g_stream.flush();
    // We flush after every line so a crash partway through the run
    // still leaves a useful tail in the log file. Trace logging is an
    // opt-in debug aid; throughput is not the priority.
}

}
