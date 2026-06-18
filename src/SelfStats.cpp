#include "SelfStats.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <mach/mach_init.h>
#include <mach/task.h>
#include <mach/task_info.h>
#endif

#ifdef __linux__
#include <linux/limits.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#include "ProcParsers.hpp"
#include "SystemMetrics.hpp"
#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pillar E of the v0.4 roadmap: live self-monitoring. The badge in the
// footer and the new Self section on the About tab both consume
// SelfStats::read(); the implementation here keeps the per-frame cost
// to a single platform syscall plus a topProcesses() filter.
//
// macOS + Linux are fully wired. Windows is stubbed with a TODO:
// GetProcessMemoryInfo + GetModuleFileNameW are the right calls, but
// the project's Windows CI is not currently exercised on this branch
// so the safe-default Snapshot{} is what we ship.

namespace SelfStats
{
    namespace
    {
        // Footer badge x/y. The spec calls for x=600 y=492, the
        // existing footer chrome row. "N procs" (FooterLiveStats) ends
        // around x=470 and the EXIT button starts at x=820, so x=600
        // sits inside the available 350px gap. NetIoFooter renders
        // into the same band but its "DISK / NET" string is hard-
        // clipped at 36 chars and worst-case ends near x=720, which
        // still leaves headroom for our badge: caption-size text at
        // ~7 px / glyph keeps the worst case under 150 px wide so the
        // right edge of the badge lands near x=750 at the widest.
        // Caption size (14 pt) intentionally smaller than the rest of
        // the footer so the badge reads as a deliberate marketing
        // flex sitting alongside the chrome rather than competing
        // with it.
        constexpr int kFooterBadgeX = 600;
        constexpr int kFooterBadgeY = 492;

        std::once_flag g_startTimeFlag;
        std::chrono::steady_clock::time_point g_startTime;
        std::mutex g_peakMutex;
        unsigned long long g_peakRssBytes = 0;
        std::mutex g_binaryMutex;
        unsigned long long g_cachedBinaryBytes = 0;
        bool g_binaryAttempted = false;

        void ensureStartTime()
        {
            std::call_once(g_startTimeFlag, []()
            {
                g_startTime = std::chrono::steady_clock::now();
            });
        }

        unsigned long long uptimeSecondsSinceStart()
        {
            auto now = std::chrono::steady_clock::now();
            auto delta = std::chrono::duration_cast<std::chrono::seconds>(
                now - g_startTime).count();
            if (delta < 0)
            {
                return 0;
            }
            return static_cast<unsigned long long>(delta);
        }

        unsigned long long bumpPeakRss(unsigned long long currentRssBytes)
        {
            std::lock_guard<std::mutex> guard(g_peakMutex);
            if (currentRssBytes > g_peakRssBytes)
            {
                g_peakRssBytes = currentRssBytes;
            }
            return g_peakRssBytes;
        }

#ifdef __APPLE__
        // task_info(TASK_BASIC_INFO) is documented as legacy but is the
        // simplest way to get the current resident set size for the
        // running process without parsing anything. The struct exposes
        // resident_size as a vm_size_t (bytes) on every macOS version we
        // support. Failure is non-fatal; we return 0 and let the caller
        // fall through to the topProcesses filter (which it tries first
        // anyway).
        unsigned long long readMacRssBytes()
        {
            mach_task_basic_info_data_t info;
            mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
            kern_return_t kr = task_info(mach_task_self(),
                                         MACH_TASK_BASIC_INFO,
                                         reinterpret_cast<task_info_t>(&info),
                                         &count);
            if (kr != KERN_SUCCESS)
            {
                return 0;
            }
            return static_cast<unsigned long long>(info.resident_size);
        }

        std::string readMacExecutablePath()
        {
            std::vector<char> buf(1024);
            uint32_t size = static_cast<uint32_t>(buf.size());
            int rc = _NSGetExecutablePath(buf.data(), &size);
            if (rc == -1)
            {
                // Buffer too small; size now holds the required length.
                buf.assign(size, '\0');
                if (_NSGetExecutablePath(buf.data(), &size) != 0)
                {
                    return std::string();
                }
            }
            return std::string(buf.data());
        }
#endif

#ifdef __linux__
        unsigned long long readLinuxRssBytes()
        {
            std::ifstream f("/proc/self/status");
            if (!f.is_open())
            {
                return 0;
            }
            std::ostringstream oss;
            oss << f.rdbuf();
            unsigned long long kb = 0;
            if (!ProcParsers::parseProcStatusVmRssKb(oss.str(), kb))
            {
                return 0;
            }
            return kb * 1024ULL;
        }

        std::string readLinuxExecutablePath()
        {
            std::vector<char> buf(PATH_MAX + 1, '\0');
            ssize_t len = readlink("/proc/self/exe",
                                   buf.data(),
                                   buf.size() - 1);
            if (len <= 0)
            {
                return std::string();
            }
            buf[static_cast<std::size_t>(len)] = '\0';
            return std::string(buf.data());
        }
#endif

        unsigned long long readBinarySize()
        {
            std::lock_guard<std::mutex> guard(g_binaryMutex);
            if (g_binaryAttempted)
            {
                return g_cachedBinaryBytes;
            }
            g_binaryAttempted = true;

            std::string path;
#ifdef __APPLE__
            path = readMacExecutablePath();
#elif defined(__linux__)
            path = readLinuxExecutablePath();
#else
            // Windows path resolution deferred. See file header.
#endif
            if (path.empty())
            {
                return 0;
            }
            struct stat st;
            if (::stat(path.c_str(), &st) != 0)
            {
                return 0;
            }
            g_cachedBinaryBytes = static_cast<unsigned long long>(st.st_size);
            return g_cachedBinaryBytes;
        }

        // formatSelfRssCompact maps bytes to a "60M" / "1.2G" style
        // string suited to the footer width budget. K below 1 MB, M up
        // to 1 GB, G beyond. One decimal place at G-scale so a 1.6 GB
        // RSS reads as "1.6G" rather than "1G".
        std::string formatSelfRssCompact(unsigned long long bytes)
        {
            if (bytes == 0)
            {
                return "--";
            }
            const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
            if (mb >= 1024.0)
            {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%.1fG", mb / 1024.0);
                return std::string(buf);
            }
            if (mb >= 1.0)
            {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%lluM",
                              static_cast<unsigned long long>(mb + 0.5));
                return std::string(buf);
            }
            unsigned long long kb = bytes / 1024ULL;
            return std::to_string(kb) + "K";
        }

        std::string formatCpuPct(float pct)
        {
            if (pct < 0.0f)
            {
                pct = 0.0f;
            }
            if (pct > 100.0f)
            {
                pct = 100.0f;
            }
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%.1f%%", pct);
            return std::string(buf);
        }
    }

    Snapshot read()
    {
        ensureStartTime();

        Snapshot snap;
        snap.processUptimeSeconds = uptimeSecondsSinceStart();
        snap.binaryBytes = readBinarySize();

#ifndef _WIN32
        const int selfPid = ::getpid();
#else
        const int selfPid = 0;
#endif

        // Prefer the topProcesses() filter so the badge's RSS / CPU%
        // numbers stay byte-identical to the table the user can already
        // see in the Processes tab. memPct is a percentage of total RAM,
        // so we multiply by HwInfo::totalRamBytes() to land bytes. The
        // window is 32 so a midrange Mac running the live UI almost
        // always finds itself inside it; only a stressed host with a
        // long process tail falls through to the platform call below.
        std::vector<ProcessInfo> procs = SystemMetrics::topProcesses(32);
        bool found = false;
        for (const auto &p : procs)
        {
            if (p.pid == selfPid)
            {
                snap.cpuPct = p.cpuPct;
                // memPct is share of physical RAM, but we cannot reliably
                // recover the absolute byte count from a single percent
                // (precision is only 4 sig figs). Fall through to the
                // platform-specific RSS call below for the byte count,
                // which is canonical. The cpuPct sample is still the
                // useful side of the topProcesses match: it is computed
                // against the same wall-clock window the rest of the UI
                // already paints.
                found = true;
                break;
            }
        }

        unsigned long long rssBytes = 0;
#ifdef __APPLE__
        rssBytes = readMacRssBytes();
#elif defined(__linux__)
        rssBytes = readLinuxRssBytes();
#else
        // Windows: GetProcessMemoryInfo would land WorkingSetSize here.
        // Deferred until the Windows CI lane is exercised.
        rssBytes = 0;
#endif
        snap.rssBytes = rssBytes;
        snap.peakRssBytes = bumpPeakRss(rssBytes);

        // If topProcesses did not see us, we still report cpuPct of 0;
        // this is rare in practice (the live UI is always among the
        // busiest processes on the host) and keeps the snapshot honest
        // about the source of the data. found is unused after this
        // point but documents the intent.
        (void)found;
        return snap;
    }

    void renderFooterBadge(Screen &dest)
    {
        Snapshot snap = read();
        const vec3 dim = Theme::textDim();
        const vec3 accent = Theme::accentGreen();

        // Layout: "self RSS <V> CPU <V>"
        // Label fragments render in dim (Theme::textDim), numeric
        // values render in accentGreen so a reviewer's eye lands on
        // the footprint values rather than the chrome words around
        // them. Caption size keeps the worst-case string under 150 px
        // so the badge fits inside the existing footer row without
        // colliding with NetIoFooter's DISK / NET output at its 36-
        // char hard clip.
        int x = kFooterBadgeX;
        const int y = kFooterBadgeY;
        const Font::Size sz = Font::Size::Caption;

        const std::string labelSelf = "self ";
        Font::drawText(dest, labelSelf, ivec2(x, y), dim, sz);
        x += Font::measureTextWidth(labelSelf, sz);

        const std::string labelRss = "RSS ";
        Font::drawText(dest, labelRss, ivec2(x, y), dim, sz);
        x += Font::measureTextWidth(labelRss, sz);

        const std::string rssText = formatSelfRssCompact(snap.rssBytes);
        Font::drawText(dest, rssText, ivec2(x, y), accent, sz);
        x += Font::measureTextWidth(rssText, sz);

        const std::string labelCpu = " CPU ";
        Font::drawText(dest, labelCpu, ivec2(x, y), dim, sz);
        x += Font::measureTextWidth(labelCpu, sz);

        const std::string cpuText = formatCpuPct(snap.cpuPct);
        Font::drawText(dest, cpuText, ivec2(x, y), accent, sz);
    }
}
