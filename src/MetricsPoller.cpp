#include "MetricsPoller.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "AssetPath.hpp"
#include "Bar.hpp"
#include "EventManager.hpp"
#include "ProcessUtils.hpp"
#include "Row.hpp"
#include "SoundEvent.hpp"
#include "Sparkline.hpp"
#include "SystemMetrics.hpp"
#include "Thresholds.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 15 of the
// modernization roadmap. The previous in-place pollMetrics() body
// lived inside the live-UI main loop and was the single largest
// remaining extractable block in that TU; this TU keeps the same
// data ownership and consults what it does not own via extern.
//
// Globals that moved here from coremetrics.cpp so the tests binary
// (which links every src/*.cpp object but not the coremetrics.cpp
// TU) resolves the symbols MetricsPoller references. coremetrics.cpp
// now declares each of them extern at the top of the file.
//   g_cpuPct, g_ramPct, g_gpuPct,
//   g_perCoreCpu, g_memBreakdown, g_uptimeSeconds, g_loadAverages,
//   g_diskUsage, g_netIo,
//   g_aggregateDiskReadKbPerSec, g_aggregateDiskWriteKbPerSec,
//   g_lastProcCount, g_sumCpuPct, g_sumMemPct,
//   g_cpuAlarmActive, g_ramAlarmActive, g_gpuAlarmActive,
//   g_rowGlyphPrefix, g_rowGlyphCollapsed,
//   g_cpuBar, g_ramBar, g_gpuBar (scene widget pointers; the writer
//     is still cacheElementPointers() in coremetrics.cpp, the
//     pointers live here so the tests link),
//   g_cpuSparkline, g_ramSparkline, g_gpuSparkline,
//   g_netRxSparkline, g_netTxSparkline (sparkline owners; the
//     writer is still buildSparklines() / destroySparklines() in
//     coremetrics.cpp, same tests-link reasoning).
// The compareProcesses() comparator and the ALARM_THRESHOLD /
// ALARM_SOUND_PATH constants are only used by the poller, so they
// moved here as well; future slices (the wider C-pillar process
// table extraction) will likely promote the comparator to a header
// once a second TU needs it.
//
// Live-UI state owned by KeyboardEvents.cpp (slice 13) and
// MouseEvents.cpp (slice 14): the poller reads + writes those
// through the externs below. g_processRows still lives in
// KeyboardEvents.cpp (the keyboard handler is its primary writer);
// the bar + sparkline pointers moved here so the tests binary
// resolves them, but buildScene() / buildSparklines() /
// cacheElementPointers() in coremetrics.cpp remain the sole writers
// of the pointer values themselves.

// Live measurement snapshots. Written by poll(), read by the render
// path in coremetrics.cpp every frame so the System tab readouts,
// uptime row, disk row, net footer, memory breakdown strip, and
// per-core strip can paint without re-querying the backend.
float g_cpuPct = 0.0f;
float g_ramPct = 0.0f;
float g_gpuPct = 0.0f;
std::vector<float> g_perCoreCpu;
MemBreakdown g_memBreakdown{0, 0, 0, 0, 0};
unsigned long long g_uptimeSeconds = 0;
std::vector<float> g_loadAverages;
DiskUsage g_diskUsage{0, 0};
NetIo g_netIo{0, 0};
unsigned long long g_aggregateDiskReadKbPerSec = 0;
unsigned long long g_aggregateDiskWriteKbPerSec = 0;

// Process-table summary. g_lastProcCount is the post-fetch count
// (before filter), so the footer "N procs" stays meaningful even
// when a filter is active and narrows the visible window.
// g_sumCpuPct / g_sumMemPct are the summed top-N pressure values
// the Processes tab aggregate strip displays.
std::size_t g_lastProcCount = 0;
float g_sumCpuPct = 0.0f;
float g_sumMemPct = 0.0f;

// Alarm-active edge detector. poll() pushes a SoundEvent the first
// tick a metric crosses ALARM_THRESHOLD, then suppresses subsequent
// ticks until it falls back below. The three booleans are the
// "previously alarming" state for CPU / RAM / GPU.
bool g_cpuAlarmActive = false;
bool g_ramAlarmActive = false;
bool g_gpuAlarmActive = false;

// Tree-mode collapse glyph overlay state. The Row widget renders
// every cell in white; the render path in coremetrics.cpp repaints
// the leading indent + '+ ' / '- ' fragment of each row in a
// green/grey accent on top of the row text. The parallel arrays
// are sized to the visible window each poll; empty entries mean
// "leaf row or flat mode, no glyph".
std::vector<std::string> g_rowGlyphPrefix;
std::vector<bool> g_rowGlyphCollapsed;

// Scene widget pointers. Bar pointers are assigned by
// cacheElementPointers() in coremetrics.cpp once the scene tree is
// loaded; the sparkline owners are assigned by buildSparklines() /
// destroySparklines() in the same TU and may be null when
// --sparklines is off. The pointers themselves live here so the
// tests binary (which links every src/*.cpp object but not the
// coremetrics.cpp TU) resolves the symbols at link time; the data
// stays null in tests because no scene gets built. poll() drives
// setValue / setFillColor / push() on each widget every tick.
Bar *g_cpuBar = nullptr;
Bar *g_ramBar = nullptr;
Bar *g_gpuBar = nullptr;
std::unique_ptr<Sparkline> g_cpuSparkline;
std::unique_ptr<Sparkline> g_ramSparkline;
std::unique_ptr<Sparkline> g_gpuSparkline;
std::unique_ptr<Sparkline> g_netRxSparkline;
std::unique_ptr<Sparkline> g_netTxSparkline;

// Sound toggle. Defined in src/MouseEvents.cpp (slice 14). poll()
// gates the alarm-flash SoundEvent push on this flag so the SOUND
// OFF button silences the alarm without disabling the threshold
// re-color.
extern bool g_alarmEnabled;

// Sort state. Defined in src/MouseEvents.cpp (slice 14). The
// comparator below dispatches through compareProcessByColumn() so
// the active column + direction drive the table sort in flat mode.
extern SortColumn g_sortColumn;
extern bool g_sortAscending;

// Processes-tab row pointers + live-UI state machine. Defined in
// src/KeyboardEvents.cpp (slice 13). poll() reads the row table to
// know how many data rows the layout has, rewrites their cells from
// the fresh process sample, and re-anchors the selected-pid /
// selected-row highlight after the sample changes.
extern std::vector<Row *> g_processRows;
extern bool g_treeMode;
extern std::unordered_set<int> g_collapsedPids;
extern std::string g_filterText;
extern std::size_t g_processScrollOffset;
extern std::size_t g_processVisibleCount;
extern std::vector<int> g_visiblePids;
extern int g_selectedPid;
extern int g_selectedRowIndex;

namespace
{
    // Bar fill recolors at and above this percentage. Mirrors the
    // per-core strip and DISK readout palette so a reviewer reads
    // "pressure rising" consistently across the System tab.
    constexpr float kAlarmThreshold = 80.0f;
    // Click chirp played once per rising-edge crossing of the
    // alarm threshold. Resolved at TU init so the click-on-rising
    // path stays a single string field-read; the sound is muted
    // through SoundPlayer when the user toggles SOUND OFF.
    const std::string kAlarmSoundPath = AssetPath::resolve("assets/click.wav");

    // Process-table sort comparator. Lives here next to its sole
    // caller; promoted to a namespace-anonymous function (was a
    // file-static in coremetrics.cpp) without changing semantics.
    // The wider C-pillar process table extraction will lift this
    // to a public ProcessUtils helper once a second TU needs it.
    bool compareProcesses(const ProcessInfo &a, const ProcessInfo &b)
    {
        return compareProcessByColumn(a, b, g_sortColumn, g_sortAscending);
    }
}

namespace MetricsPoller
{
    void poll()
    {
        float cpuPct = SystemMetrics::readCpuPercent();
        float memPct = SystemMetrics::readMemPercent();
        float gpuPct = SystemMetrics::readGpuPercent();
        // Stash the latest percentages so the render loop can hand them to
        // MetricReadouts::render(...) outside of this scope.
        g_cpuPct = cpuPct;
        g_ramPct = memPct;
        g_gpuPct = gpuPct;

        // Threshold-based fill colors mirror the per-core strip + DISK
        // readout palette: accent green idle, yellow at 60%+, red at 80%+.
        // Re-colors all three bars consistently so the System tab reads
        // 'pressure rising' across CPU / RAM / GPU at a glance.
        if (g_cpuBar != nullptr)
        {
            g_cpuBar->setValue(cpuPct);
            g_cpuBar->setFillColor(Thresholds::colorForPct(cpuPct));
        }
        if (g_ramBar != nullptr)
        {
            g_ramBar->setValue(memPct);
            g_ramBar->setFillColor(Thresholds::colorForPct(memPct));
        }
        if (g_gpuBar != nullptr)
        {
            g_gpuBar->setValue(gpuPct);
            g_gpuBar->setFillColor(Thresholds::colorForPct(gpuPct));
        }
        if (g_cpuSparkline != nullptr)
        {
            g_cpuSparkline->push(cpuPct);
        }
        if (g_ramSparkline != nullptr)
        {
            g_ramSparkline->push(memPct);
        }
        if (g_gpuSparkline != nullptr)
        {
            g_gpuSparkline->push(gpuPct);
        }

        g_perCoreCpu = SystemMetrics::readPerCoreCpu();
        g_memBreakdown = SystemMetrics::readMemBreakdown();
        g_uptimeSeconds = SystemMetrics::readUptimeSeconds();
        g_loadAverages = SystemMetrics::readLoadAverages();
        g_diskUsage = SystemMetrics::readDiskUsage();

        bool cpuNowAlarm = cpuPct >= kAlarmThreshold;
        bool ramNowAlarm = memPct >= kAlarmThreshold;
        bool gpuNowAlarm = gpuPct >= kAlarmThreshold;
        if (g_alarmEnabled)
        {
            if (cpuNowAlarm && !g_cpuAlarmActive)
            {
                EventManager::getInstance().pushEvent(std::make_unique<SoundEvent>(kAlarmSoundPath));
            }
            if (ramNowAlarm && !g_ramAlarmActive)
            {
                EventManager::getInstance().pushEvent(std::make_unique<SoundEvent>(kAlarmSoundPath));
            }
            if (gpuNowAlarm && !g_gpuAlarmActive)
            {
                EventManager::getInstance().pushEvent(std::make_unique<SoundEvent>(kAlarmSoundPath));
            }
        }
        g_cpuAlarmActive = cpuNowAlarm;
        g_ramAlarmActive = ramNowAlarm;
        g_gpuAlarmActive = gpuNowAlarm;

        if (g_processRows.size() <= 1)
        {
            return;
        }

        std::size_t dataRowCount = g_processRows.size() - 1;
        // Tree mode walks the parent/child graph; if parents fall outside the
        // top-N memory window the chain breaks and most rows render at depth 0,
        // so we fetch a much larger sample when tree mode is on. Flat mode's
        // 3x over-fetch is enough for the substring filter alone.
        std::size_t fetchN = g_treeMode ? std::max<std::size_t>(500, dataRowCount * 30)
                                        : dataRowCount * 3;
        std::vector<ProcessInfo> procs = SystemMetrics::topProcesses(fetchN);
        // Captured before filter/truncate so the live footer count reflects
        // what the backend actually returned (after any over-fetch), which
        // is the most useful single number a reviewer can see: "this system
        // monitor is watching N processes right now."
        g_lastProcCount = procs.size();
        // Aggregate disk-I/O throughput summed across every process the
        // backend returned this round. Renders as 'R X.X MB/s  W Y.Y MB/s'
        // text strip on the System tab so users see system-wide disk
        // pressure at a glance instead of having to scan the Processes
        // table. Best-effort: only counts processes the backend surfaced
        // (top fetchN by memory), which captures the loudest writers in
        // practice without needing /proc/diskstats or PDH PhysicalDisk.
        unsigned long long aggReadKb = 0;
        unsigned long long aggWriteKb = 0;
        float sumCpuPct = 0.0f;
        float sumMemPct = 0.0f;
        for (const auto &p : procs)
        {
            aggReadKb += p.diskReadKbPerSec;
            aggWriteKb += p.diskWriteKbPerSec;
            sumCpuPct += p.cpuPct;
            sumMemPct += p.memPct;
        }
        g_aggregateDiskReadKbPerSec = aggReadKb;
        g_aggregateDiskWriteKbPerSec = aggWriteKb;
        // Cached for ProcessesSummary::render so the strip can paint each frame
        // without rewalking the process list. Stale by at most one poll tick.
        g_sumCpuPct = sumCpuPct;
        g_sumMemPct = sumMemPct;
        // Sample aggregate net I/O on the same cadence as the rest of the
        // System tab data. First call returns zeros (no prior sample).
        g_netIo = SystemMetrics::readNetIo();
        // Feed the same sample into the rx/tx polylines so the NET history
        // strip mirrors the live footer 'NET R/T' readout one tick later.
        if (g_netRxSparkline != nullptr)
        {
            g_netRxSparkline->push(static_cast<float>(g_netIo.rxKbPerSec));
        }
        if (g_netTxSparkline != nullptr)
        {
            g_netTxSparkline->push(static_cast<float>(g_netIo.txKbPerSec));
        }
        // Parallel array of indent depths produced when tree mode flattens
        // the parent/child graph; stays empty in flat mode. Same length as
        // procs after the sort+filter+truncate pass below.
        std::vector<int> procDepth;
        // Parallel flag: row has at least one child in the current sample,
        // so its name cell should carry a '+' or '-' collapse glyph. Stays
        // empty in flat mode. Same length policy as procDepth.
        std::vector<bool> procHasChildren;
        if (g_treeMode)
        {
            // Build a parent -> children index. Roots are entries whose parentPid
            // isn't itself a pid we have a record for. Children inside each
            // bucket sort by memPct desc so the heaviest child surfaces first.
            std::unordered_map<int, std::vector<std::size_t>> childIndices;
            std::unordered_set<int> knownPids;
            knownPids.reserve(procs.size());
            for (const auto &p : procs)
            {
                knownPids.insert(p.pid);
            }
            for (std::size_t i = 0; i < procs.size(); ++i)
            {
                childIndices[procs[i].parentPid].push_back(i);
            }
            for (auto &kv : childIndices)
            {
                std::sort(kv.second.begin(), kv.second.end(),
                    [&procs](std::size_t a, std::size_t b) {
                        return procs[a].memPct > procs[b].memPct;
                    });
            }

            std::vector<ProcessInfo> flat;
            flat.reserve(procs.size());
            procDepth.reserve(procs.size());
            procHasChildren.reserve(procs.size());

            // Iterative DFS so we don't recurse into a kernel-thread chain that
            // could be thousands deep on weird hosts.
            std::vector<std::pair<std::size_t, int>> stack;
            for (std::size_t i = 0; i < procs.size(); ++i)
            {
                if (knownPids.find(procs[i].parentPid) == knownPids.end())
                {
                    stack.push_back({i, 0});
                }
            }
            // Roots also sort by mem desc so the densest tree surfaces first.
            std::sort(stack.begin(), stack.end(),
                [&procs](const std::pair<std::size_t, int> &a,
                         const std::pair<std::size_t, int> &b) {
                    return procs[a.first].memPct > procs[b.first].memPct;
                });
            std::vector<bool> emitted(procs.size(), false);
            while (!stack.empty())
            {
                auto [idx, depth] = stack.back();
                stack.pop_back();
                if (emitted[idx])
                {
                    continue;
                }
                emitted[idx] = true;
                flat.push_back(procs[idx]);
                procDepth.push_back(depth);
                // Cache whether this pid has any child in the current sample
                // so the render loop can decide whether to draw a +/- glyph.
                // Checked against childIndices BEFORE the collapse short-circuit
                // so collapsed parents still get a glyph (the '+').
                auto childIt = childIndices.find(procs[idx].pid);
                bool hasChildren = (childIt != childIndices.end()
                                    && !childIt->second.empty());
                procHasChildren.push_back(hasChildren);
                // Skip descendants when this pid is in the collapsed set so
                // 'space' on a row hides the subtree without dropping the row
                // itself. Set is empty by default => fully expanded tree.
                if (g_collapsedPids.count(procs[idx].pid) != 0)
                {
                    continue;
                }
                if (hasChildren)
                {
                    // Push in reverse so the highest-mem child is popped first.
                    for (auto rit = childIt->second.rbegin(); rit != childIt->second.rend(); ++rit)
                    {
                        if (!emitted[*rit])
                        {
                            stack.push_back({*rit, depth + 1});
                        }
                    }
                }
            }
            // Append any procs the DFS missed (cycles, ppid pointing at an
            // already-emitted node, etc.) at depth 0 so the table never
            // silently drops rows.
            for (std::size_t i = 0; i < procs.size(); ++i)
            {
                if (!emitted[i])
                {
                    flat.push_back(procs[i]);
                    procDepth.push_back(0);
                    procHasChildren.push_back(false);
                }
            }
            procs = std::move(flat);
        }
        else
        {
            std::sort(procs.begin(), procs.end(), compareProcesses);
        }
        if (!g_filterText.empty())
        {
            std::vector<ProcessInfo> kept;
            kept.reserve(procs.size());
            for (const auto &p : procs)
            {
                if (processNameMatchesFilter(p.name, g_filterText))
                {
                    kept.push_back(p);
                }
            }
            procs = std::move(kept);
        }
        // Stash the post-filter size before truncating so the scroll
        // clamp + the "N more below" indicator can use it.
        g_processVisibleCount = procs.size();
        // Clamp the scroll offset against the current table length so a
        // shrinking window of matching processes never leaves a stale
        // offset pointing past the end.
        if (g_processVisibleCount <= dataRowCount)
        {
            g_processScrollOffset = 0;
        }
        else
        {
            std::size_t maxOffset = g_processVisibleCount - dataRowCount;
            if (g_processScrollOffset > maxOffset)
            {
                g_processScrollOffset = maxOffset;
            }
        }
        // Slice the visible window of dataRowCount entries starting at
        // g_processScrollOffset so up/down + PgUp/PgDown move through the
        // full process list instead of being capped at the first page.
        if (g_processScrollOffset > 0)
        {
            procs.erase(procs.begin(),
                        procs.begin() + static_cast<std::ptrdiff_t>(g_processScrollOffset));
        }
        if (procs.size() > dataRowCount)
        {
            procs.resize(dataRowCount);
        }

        g_visiblePids.clear();
        g_visiblePids.reserve(procs.size());
        g_rowGlyphPrefix.assign(procs.size(), std::string());
        g_rowGlyphCollapsed.assign(procs.size(), false);

        for (std::size_t i = 0; i < dataRowCount; ++i)
        {
            Row *row = g_processRows[i + 1];
            if (row == nullptr)
            {
                continue;
            }
            if (i < procs.size())
            {
                const ProcessInfo &info = procs[i];
                g_visiblePids.push_back(info.pid);
                // Tree mode: prefix the NAME cell with indent + connector so
                // the parent/child relationship is visible. Cap depth at 6 to
                // avoid pushing the name out of its column on a deeply nested
                // chain (which is rare in practice). Nodes that have children
                // get a '+ ' (collapsed) or '- ' (expanded) glyph in the
                // indent area; leaf rows fall back to the previous '|- '
                // connector so the parent/child line stays readable.
                std::string nameCell = info.name;
                if (g_treeMode && i < procDepth.size())
                {
                    int depth = procDepth[i];
                    if (depth > 6) depth = 6;
                    std::string prefix;
                    for (int d = 0; d < depth; ++d)
                    {
                        prefix += "  ";
                    }
                    bool hasChildren = (i < procHasChildren.size()) && procHasChildren[i];
                    bool collapsed = (g_collapsedPids.count(info.pid) != 0);
                    if (hasChildren)
                    {
                        prefix += collapsed ? "+ " : "- ";
                        // Stash the colored-overlay prefix so the render
                        // pass can repaint the glyph in green/grey on top
                        // of the Row's default white text.
                        g_rowGlyphPrefix[i] = prefix;
                        g_rowGlyphCollapsed[i] = collapsed;
                    }
                    else if (depth > 0)
                    {
                        prefix += "|- ";
                    }
                    nameCell = prefix + info.name;
                }
                std::vector<std::string> cells = {
                    std::to_string(info.pid),
                    nameCell,
                    formatPct(info.cpuPct),
                    formatPct(info.memPct),
                    formatDiskIo(info.diskReadKbPerSec, info.diskWriteKbPerSec)
                };
                row->setCells(cells);
            }
            else
            {
                row->setCells({"", "", "", "", ""});
            }
        }

        // After a re-poll the visible row index for the selected pid may have
        // moved or the pid may have exited. Re-anchor the highlight so up/down
        // navigation keeps making sense.
        if (g_selectedPid >= 0)
        {
            auto it = std::find(g_visiblePids.begin(), g_visiblePids.end(), g_selectedPid);
            if (it == g_visiblePids.end())
            {
                g_selectedPid = -1;
                g_selectedRowIndex = -1;
            }
            else
            {
                g_selectedRowIndex = static_cast<int>(it - g_visiblePids.begin());
            }
        }
    }
}
