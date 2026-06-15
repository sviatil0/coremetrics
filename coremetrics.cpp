#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "screen.hpp"
#include "LayoutManager.hpp"
#include "EventManager.hpp"
#include "ClickEvent.hpp"
#include "ShowEvent.hpp"
#include "SoundEvent.hpp"
#include "SoundPlayer.hpp"
#include "font.hpp"
#include "GUIElements.hpp"
#include "GUIFile.hpp"
#include "SystemMetrics.hpp"
#include "LayoutUtils.hpp"
#include "ProcessUtils.hpp"
#include "Sparkline.hpp"
#include "AssetPath.hpp"
#include "SignalUtils.hpp"

constexpr int RESX = 960;
constexpr int RESY = 540;
constexpr int PROCESS_ROW_HEIGHT = 20;
constexpr Uint64 POLL_INTERVAL_MS = 500;

constexpr float ALARM_THRESHOLD = 80.0f;
static const std::string ALARM_SOUND_PATH = AssetPath::resolve("assets/click.wav");

static const vec3 COLOR_ACCENT_GREEN(0.871f, 1.0f, 0.608f);
static const vec3 COLOR_WHITE(1.0f, 1.0f, 1.0f);
static const vec3 COLOR_FRAME(0.85f, 0.85f, 0.85f);

static const vec3 COLOR_TAB_ACTIVE(0.10f, 0.10f, 0.10f);
static const vec3 COLOR_MUTE_BTN(0.08f, 0.08f, 0.08f);
static const vec3 COLOR_EXIT_BTN(0.18f, 0.05f, 0.05f);
static const vec3 COLOR_TEXT_PRIMARY(1.0f, 1.0f, 1.0f);
static const vec3 COLOR_TEXT_ACCENT(0.871f, 1.0f, 0.608f);
static const vec3 COLOR_BAR_CPU_FILL(0.871f, 1.0f, 0.608f);
static const vec3 COLOR_BAR_RAM_FILL(0.871f, 1.0f, 0.608f);
static const vec3 COLOR_BAR_BG(0.12f, 0.12f, 0.12f);
static const vec3 COLOR_ROW_TEXT(1.0f, 1.0f, 1.0f);
static const vec3 COLOR_ROW_HEADER(0.871f, 1.0f, 0.608f);

static Bar *g_cpuBar = nullptr;
static Bar *g_ramBar = nullptr;
static Bar *g_gpuBar = nullptr;
static Label *g_cpuReadout = nullptr;
static Label *g_ramReadout = nullptr;
static Label *g_gpuReadout = nullptr;
static Label *g_muteLabel = nullptr;
static std::vector<Row *> g_processRows;

static bool g_alarmEnabled = true;
static bool g_cpuAlarmActive = false;
static bool g_ramAlarmActive = false;
static bool g_gpuAlarmActive = false;

// Set by SIGINT/SIGTERM and checked by the main loop. The main loop also
// exits on its own once an optional --duration is exceeded; this flag covers
// Ctrl-C from a terminal and `kill` from a parent process or CI.
static std::atomic<bool> g_shutdownRequested{false};

static void handleShutdownSignal(int)
{
    g_shutdownRequested.store(true);
}

static SortColumn g_sortColumn = SORT_MEM;
static bool g_sortAscending = false;
static Row *g_headerRow = nullptr;

// Sparklines are moonshot UI: a rolling time-series chart per metric drawn
// over the System tab's lower half. Behind a flag (--sparklines) so the
// default install matches the audit baseline; with the flag the System tab
// gains 3 live polylines fed by RingBuffer<float> samples.
static bool g_sparklinesEnabled = false;
constexpr std::size_t SPARKLINE_CAPACITY = 64;
static Sparkline *g_cpuSparkline = nullptr;
static Sparkline *g_ramSparkline = nullptr;
static Sparkline *g_gpuSparkline = nullptr;

// Per-logical-CPU utilization for the small strip below the aggregate bars.
// Refreshed every poll. Empty on Windows (backend not implemented yet) and
// on the very first poll (no prior tick sample to diff against).
static std::vector<float> g_perCoreCpu;

// Process kill flow state.
//
// Mouse click on a row in the Processes tab sets g_selectedPid to that
// row's pid; up/down keys move the selection one row at a time within
// the bounds of g_visiblePids. 'k' opens the signal menu, 1-6 picks a
// signal, Y/Enter confirms send, N/Esc cancels. The menu and the per-row
// highlight are painted after the LayoutManager render so they sit on
// top of the Processes table without modifying the Row widget.
constexpr int PROCESSES_FIRST_ROW_Y = 100;
constexpr int PROCESSES_ROW_X0 = 24;
constexpr int PROCESSES_ROW_X1 = 936;
constexpr int PROCESSES_VISIBLE_ROWS = 15;

static int g_selectedPid = -1;
static int g_selectedRowIndex = -1;
static std::vector<int> g_visiblePids;

static bool g_signalMenuVisible = false;
static int g_signalMenuPickedIndex = -1; // -1 = picking, 0..5 = awaiting confirm

static std::string g_statusFlash;
static Uint64 g_statusFlashExpiryMs = 0;
constexpr Uint64 STATUS_FLASH_DURATION_MS = 2500;

// Process search/filter. '/' on the Processes tab enters input mode.
// g_filterText is the case-insensitive substring matched against process
// names; an empty string disables filtering. g_filterInputActive tracks
// whether keystrokes append to the filter (input mode) or trigger the
// usual row navigation (filter applied but no longer being edited).
static std::string g_filterText;
static bool g_filterInputActive = false;

// Tree mode: when on, the Processes table groups parent/child pairs
// depth-first and indents the name cell with tree connectors. Toggled
// by 't' on the Processes tab. When off the table sorts by the active
// column the way it has since the SAFE wave.
static bool g_treeMode = false;

static void flashStatus(const std::string &text)
{
    g_statusFlash = text;
    g_statusFlashExpiryMs = SDL_GetTicks() + STATUS_FLASH_DURATION_MS;
}

static bool processesTabActive()
{
    Tree<Layout> *node = EventManager::findLayoutByName(
        LayoutManager::getInstance().getRoot(), "processes");
    return node != nullptr && node->getData().isActive();
}

static void closeSignalMenu()
{
    g_signalMenuVisible = false;
    g_signalMenuPickedIndex = -1;
}
constexpr int PERCORE_Y0 = 218;
constexpr int PERCORE_Y1 = 236;
// Start the strip at x=84 so a 'cores' label fits at x=24, mirroring
// the CPU / RAM / GPU labels above. The previous x=24 start used the
// full row width but left the strip unlabeled, so a reviewer reading
// fast saw three bars + an unexplained mosaic.
constexpr int PERCORE_X0 = 84;
constexpr int PERCORE_X1 = 936;
constexpr int PERCORE_GAP = 4;

// Memory breakdown segmented bar. Sits just below the aggregate RAM bar
// (y=136..160 in base.xml). Shows active / wired / cached / free segments
// at the same horizontal extent so they read as a continuation. htop
// colors: active = red, wired = orange, cached = blue, free = dark.
static MemBreakdown g_memBreakdown{0, 0, 0, 0, 0};
static unsigned long long g_uptimeSeconds = 0;
static std::vector<float> g_loadAverages;
// Root volume capacity sampled every poll. UI shows
// "DISK <used>/<total> GB (NN%)" with the same yellow/red threshold
// recolor as the RAM bar so a full root volume reads as visually
// urgent. Zero totalKb means the backend failed; the strip is hidden.
static DiskUsage g_diskUsage{0, 0};
constexpr int MEMSEG_X0 = 84;
constexpr int MEMSEG_X1 = 864;
// Slim strip (164..172) so the breakdown reads as a continuation of
// the RAM bar above, not a second RAM bar stacked beneath. The earlier
// 14px height made the two bars feel like duplicate metrics; 8px keeps
// the segment colors readable while letting the eye treat the strip
// as a thin annotation rather than its own row.
constexpr int MEMSEG_Y0 = 164;
constexpr int MEMSEG_Y1 = 172;

static ivec2 g_headerColMin[5];
static ivec2 g_headerColMax[5];
static ivec2 g_muteBtnMin;
static ivec2 g_muteBtnMax;
static ivec2 g_exitBtnMin;
static ivec2 g_exitBtnMax;

static bool compareProcesses(const ProcessInfo &a, const ProcessInfo &b)
{
    return compareProcessByColumn(a, b, g_sortColumn, g_sortAscending);
}

static void buildScene()
{
    LayoutManager &manager = LayoutManager::getInstance();
    GUIFile g;
    g.readFile(AssetPath::resolve("base.xml"), manager);

    g_muteBtnMin = ivec2(812, 8);
    g_muteBtnMax = ivec2(952, 40);

    g_exitBtnMin = ivec2(880, 460);
    g_exitBtnMax = ivec2(944, 524);

    int rowY = 64;
    std::vector<float> weights = {0.10f, 0.42f, 0.12f, 0.12f, 0.24f};
    int headerRowWidth = 936 - 24;
    int colX = 24;
    for (int c = 0; c < 5; ++c)
    {
        int colW = static_cast<int>(weights[c] * static_cast<float>(headerRowWidth));
        g_headerColMin[c] = ivec2(colX, rowY);
        g_headerColMax[c] = ivec2(colX + colW, rowY + PROCESS_ROW_HEIGHT);
        colX += colW;
    }
}

static void cacheElementPointers()
{
    Tree<Layout> &root = LayoutManager::getInstance().getRoot();
    g_cpuBar = findBarByMetric(root, "cpu");
    g_ramBar = findBarByMetric(root, "ram");

    Tree<Layout> *systemNode = EventManager::findLayoutByName(root, "system");
    if (systemNode != nullptr)
    {
        g_cpuReadout = nthLabelInLayout(*systemNode, 1);
        g_ramReadout = nthLabelInLayout(*systemNode, 3);
        g_gpuReadout = nthLabelInLayout(*systemNode, 5);
    }
    g_gpuBar = findBarByMetric(root, "gpu");

    Tree<Layout> *tabbarNode = EventManager::findLayoutByName(root, "tabbar");
    if (tabbarNode != nullptr)
    {
        g_muteLabel = nthLabelInLayout(*tabbarNode, 2);
    }

    g_processRows.clear();
    collectRows(root, g_processRows);

    if (!g_processRows.empty())
    {
        g_headerRow = g_processRows.front();
    }
}

static void buildSparklines()
{
    // Stacked sparklines in the System tab's empty lower half. Each row spans
    // the same horizontal range as the bars above (x in [24, 864]) and
    // matches the same accent color, so the polyline reads as a continuation
    // of its bar. Heights are 56px / row with 12px gaps.
    const vec3 accent(0.871f, 1.0f, 0.608f);
    g_cpuSparkline = new Sparkline(ivec2(24, 240), ivec2(864, 296), accent,
                                   0.0f, 100.0f, SPARKLINE_CAPACITY);
    g_ramSparkline = new Sparkline(ivec2(24, 308), ivec2(864, 364), accent,
                                   0.0f, 100.0f, SPARKLINE_CAPACITY);
    g_gpuSparkline = new Sparkline(ivec2(24, 376), ivec2(864, 432), accent,
                                   0.0f, 100.0f, SPARKLINE_CAPACITY);
}

static void destroySparklines()
{
    delete g_cpuSparkline;
    delete g_ramSparkline;
    delete g_gpuSparkline;
    g_cpuSparkline = nullptr;
    g_ramSparkline = nullptr;
    g_gpuSparkline = nullptr;
}

static std::string formatUptime(unsigned long long secs)
{
    return formatUptimeString(secs);
}

static std::string formatLoadAverages()
{
    if (g_loadAverages.size() < 3)
    {
        return "Load --";
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Load %.2f %.2f %.2f",
                  g_loadAverages[0], g_loadAverages[1], g_loadAverages[2]);
    return std::string(buf);
}

static void renderUptimeAndLoad(Screen &dest)
{
    // Dim white for the labels, accent green so it reads as a status row.
    const vec3 dimColor(0.55f, 0.55f, 0.55f);
    Font::drawText(dest, formatUptime(g_uptimeSeconds), ivec2(24, 44), dimColor);
    // Uptime + Load + Disk all share the y=44 baseline so the status
    // row reads as a single line instead of a staircase. The earlier
    // y=56 placement for Load predated the DISK strip and left Load
    // ~12 px below Up, which was visible as a misaligned middle field.
    Font::drawText(dest, formatLoadAverages(), ivec2(220, 44), dimColor);
}

static void renderDiskUsage(Screen &dest)
{
    if (g_diskUsage.totalKb == 0)
    {
        return;
    }
    unsigned long long usedKb = (g_diskUsage.totalKb > g_diskUsage.freeKb)
                                    ? g_diskUsage.totalKb - g_diskUsage.freeKb
                                    : 0;
    float pct = computeDiskUsedPct(g_diskUsage.totalKb, g_diskUsage.freeKb);

    vec3 color(0.55f, 0.55f, 0.55f);
    if (pct >= 80.0f) color = vec3(0.95f, 0.35f, 0.35f);
    else if (pct >= 60.0f) color = vec3(0.95f, 0.82f, 0.40f);

    std::string text = "DISK " + formatGbString(usedKb) + " / "
                       + formatGbString(g_diskUsage.totalKb) + " GB ("
                       + formatPct(pct) + "%)";
    // Right side of the status row at y=44, mirroring the Up + Load
    // string on the left. x=560 leaves room for a long string ("DISK
    // 1457 / 2048 GB (71.1%)") without clipping the SOUND ON button at
    // x=812.
    Font::drawText(dest, text, ivec2(560, 44), color);
}

static void renderMemBreakdownStrip(Screen &dest)
{
    if (g_memBreakdown.totalKb == 0)
    {
        return;
    }
    const int width = MEMSEG_X1 - MEMSEG_X0;
    if (width <= 0)
    {
        return;
    }

    auto segPx = [&](unsigned long long kb) {
        // Compute as 64-bit to avoid overflowing the multiplication on
        // large-memory hosts (256GB+).
        unsigned long long n = static_cast<unsigned long long>(width) * kb;
        return static_cast<int>(n / g_memBreakdown.totalKb);
    };

    const vec3 colActive(0.95f, 0.35f, 0.35f); // red: in-use by processes
    const vec3 colWired(0.95f, 0.66f, 0.30f);  // orange: kernel + pinned
    const vec3 colCached(0.45f, 0.78f, 0.95f); // blue: reclaimable cache
    const vec3 colFree(0.18f, 0.18f, 0.18f);   // dark: free

    int x = MEMSEG_X0;
    // Active
    int aw = segPx(g_memBreakdown.activeKb);
    if (aw > 0) { dest.drawBox(ivec2(x, MEMSEG_Y0), ivec2(x + aw, MEMSEG_Y1), colActive); x += aw; }
    // Wired
    int ww = segPx(g_memBreakdown.wiredKb);
    if (ww > 0) { dest.drawBox(ivec2(x, MEMSEG_Y0), ivec2(x + ww, MEMSEG_Y1), colWired); x += ww; }
    // Cached
    int cw = segPx(g_memBreakdown.cachedKb);
    if (cw > 0) { dest.drawBox(ivec2(x, MEMSEG_Y0), ivec2(x + cw, MEMSEG_Y1), colCached); x += cw; }
    // Free fills whatever is left so rounding remainder gets absorbed visually.
    if (x < MEMSEG_X1)
    {
        dest.drawBox(ivec2(x, MEMSEG_Y0), ivec2(MEMSEG_X1, MEMSEG_Y1), colFree);
    }

}

static void renderPerCoreStrip(Screen &dest)
{
    if (g_perCoreCpu.empty())
    {
        return;
    }
    // Left-edge label matches the CPU / RAM / GPU column style. Text
    // is rendered with its top-left anchor at (24, 218); the bundled
    // 20px font puts the glyph bowl inside the 218..236 strip vertical
    // band so the label aligns with the cell centers.
    const vec3 labelColor(0.55f, 0.55f, 0.55f);
    Font::drawText(dest, "cores", ivec2(24, 218), labelColor);
    const std::size_t cores = g_perCoreCpu.size();
    const int totalGap = PERCORE_GAP * static_cast<int>(cores - 1);
    const int slot = (PERCORE_X1 - PERCORE_X0 - totalGap) / static_cast<int>(cores);
    if (slot <= 0)
    {
        return;
    }
    const vec3 bg(0.12f, 0.12f, 0.12f);
    const vec3 fillLow(0.871f, 1.0f, 0.608f);   // accent green
    const vec3 fillMid(0.95f, 0.82f, 0.40f);    // yellow at 60%+
    const vec3 fillHigh(0.95f, 0.35f, 0.35f);   // red at 80%+

    for (std::size_t i = 0; i < cores; ++i)
    {
        int x0 = PERCORE_X0 + static_cast<int>(i) * (slot + PERCORE_GAP);
        int x1 = x0 + slot;
        dest.drawBox(ivec2(x0, PERCORE_Y0), ivec2(x1, PERCORE_Y1), bg);

        float ratio = g_perCoreCpu[i] / 100.0f;
        if (ratio <= 0.0f)
        {
            continue;
        }
        if (ratio > 1.0f) ratio = 1.0f;
        int fillW = static_cast<int>(ratio * static_cast<float>(slot));
        if (fillW <= 0)
        {
            continue;
        }
        vec3 color = fillLow;
        if (ratio > 0.8f) color = fillHigh;
        else if (ratio > 0.6f) color = fillMid;
        dest.drawBox(ivec2(x0, PERCORE_Y0), ivec2(x0 + fillW, PERCORE_Y1), color);
    }
}

// Live footer string replacing the v0.1.x static "alarm threshold 80%"
// label. Same color and y-baseline as the other footer chrome
// (coremetrics / v0.2.0 / poll 500ms) so it reads as a continuation.
// Latest process count is set by pollMetrics().
static std::size_t g_lastProcCount = 0;

static void renderFooterLiveStats(Screen &dest)
{
    const vec3 dim(0.5f, 0.5f, 0.5f);
    std::string text = "procs " + std::to_string(g_lastProcCount);
    Font::drawText(dest, text, ivec2(370, 492), dim);
}

// Tiny accent labels at the left edge of each sparkline strip. Without
// them three unlabeled polylines at the bottom of the System tab read as
// abstract decoration; with them a reviewer instantly sees they're CPU
// / RAM / GPU history. y-baseline picked to sit just above each strip.
static void renderSparklineLabels(Screen &dest)
{
    const vec3 dim(0.55f, 0.55f, 0.55f);
    Font::drawText(dest, "CPU history", ivec2(24, 230), dim);
    Font::drawText(dest, "RAM history", ivec2(24, 298), dim);
    Font::drawText(dest, "GPU history", ivec2(24, 366), dim);
}

static void pollMetrics()
{
    float cpuPct = SystemMetrics::readCpuPercent();
    float memPct = SystemMetrics::readMemPercent();
    float gpuPct = SystemMetrics::readGpuPercent();

    if (g_cpuBar != nullptr)
    {
        g_cpuBar->setValue(cpuPct);
    }
    if (g_ramBar != nullptr)
    {
        g_ramBar->setValue(memPct);
    }
    if (g_gpuBar != nullptr)
    {
        g_gpuBar->setValue(gpuPct);
    }
    if (g_cpuReadout != nullptr)
    {
        g_cpuReadout->setText(formatPct(cpuPct) + "%");
    }
    if (g_ramReadout != nullptr)
    {
        g_ramReadout->setText(formatPct(memPct) + "%");
    }
    if (g_gpuReadout != nullptr)
    {
        g_gpuReadout->setText(formatPct(gpuPct) + "%");
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

    bool cpuNowAlarm = cpuPct >= ALARM_THRESHOLD;
    bool ramNowAlarm = memPct >= ALARM_THRESHOLD;
    bool gpuNowAlarm = gpuPct >= ALARM_THRESHOLD;
    if (g_alarmEnabled)
    {
        if (cpuNowAlarm && !g_cpuAlarmActive)
        {
            EventManager::getInstance().pushEvent(std::make_unique<SoundEvent>(ALARM_SOUND_PATH));
        }
        if (ramNowAlarm && !g_ramAlarmActive)
        {
            EventManager::getInstance().pushEvent(std::make_unique<SoundEvent>(ALARM_SOUND_PATH));
        }
        if (gpuNowAlarm && !g_gpuAlarmActive)
        {
            EventManager::getInstance().pushEvent(std::make_unique<SoundEvent>(ALARM_SOUND_PATH));
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
    // Parallel array of indent depths produced when tree mode flattens
    // the parent/child graph; stays empty in flat mode. Same length as
    // procs after the sort+filter+truncate pass below.
    std::vector<int> procDepth;
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
            auto it = childIndices.find(procs[idx].pid);
            if (it != childIndices.end())
            {
                // Push in reverse so the highest-mem child is popped first.
                for (auto rit = it->second.rbegin(); rit != it->second.rend(); ++rit)
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
    if (procs.size() > dataRowCount)
    {
        procs.resize(dataRowCount);
    }

    g_visiblePids.clear();
    g_visiblePids.reserve(procs.size());

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
            // chain (which is rare in practice).
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
                if (depth > 0)
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

int main(int argc, char **argv)
{
    // Optional headless screenshot mode: `coremetrics --screenshot out.bmp`
    // renders one frame to an offscreen surface and saves it, no window needed.
    std::string screenshotPath;
    // Optional `--duration <seconds>` cleanly exits the live UI after N
    // seconds. Useful for backgrounded smoke tests and screenshot capture
    // pipelines that need a guaranteed-finite run. 0 means "run forever".
    double durationSeconds = 0.0;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--screenshot" && i + 1 < argc)
        {
            screenshotPath = argv[i + 1];
        }
        if (std::string(argv[i]) == "--duration" && i + 1 < argc)
        {
            durationSeconds = std::atof(argv[i + 1]);
            if (durationSeconds < 0.0)
            {
                durationSeconds = 0.0;
            }
        }
        if (std::string(argv[i]) == "--sparklines")
        {
            g_sparklinesEnabled = true;
        }
        // --tree opens the Processes tab in parent/child indented hierarchy
        // mode, same as pressing 't' on that tab interactively. Lets the
        // screenshot pipeline capture tree mode without simulating keystrokes.
        if (std::string(argv[i]) == "--tree")
        {
            g_treeMode = true;
        }
        // --filter <substring> seeds the Processes-tab name filter so the
        // screenshot pipeline can demonstrate search. Case-insensitive
        // substring match against process names, applied during pollMetrics.
        if (std::string(argv[i]) == "--filter" && i + 1 < argc)
        {
            g_filterText = argv[i + 1];
        }
    }

    std::signal(SIGINT, handleShutdownSignal);
    std::signal(SIGTERM, handleShutdownSignal);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cerr << "Failed to init SDL: " << SDL_GetError() << '\n';
        return -1;
    }

    if (!screenshotPath.empty())
    {
        // Optional second arg selects the tab: --screenshot out.bmp processes
        std::string tab = "system";
        for (int i = 1; i < argc - 1; ++i)
        {
            if (std::string(argv[i]) == "--screenshot" && i + 2 < argc)
            {
                tab = argv[i + 2];
            }
        }

        Screen shot(RESX, RESY);
        buildScene();
        cacheElementPointers();
        if (g_sparklinesEnabled)
        {
            buildSparklines();
            // Prime the sparklines so a single headless --screenshot frame has
            // enough history to draw a visible polyline. 64 samples at 50ms
            // each (~3.2s) fills the rolling window exactly and also gives the
            // per-process CPU% delta a wide enough sampling window that idle
            // processes still register measurable ticks.
            for (std::size_t i = 0; i < SPARKLINE_CAPACITY; ++i)
            {
                pollMetrics();
                SDL_Delay(50);
            }
        }
        else
        {
            // Per-process CPU% is a delta between two samples. 1.1s was too
            // tight: many processes accumulate sub-tick activity in that
            // window and round to 0.0% in the shot. 3s captures everything
            // that is doing real work without making the screenshot path
            // feel sluggish.
            pollMetrics();
            SDL_Delay(3000);
        }
        pollMetrics();

        if (tab == "processes")
        {
            EventManager::getInstance().pushEvent(std::make_unique<ShowEvent>("system", false));
            EventManager::getInstance().pushEvent(std::make_unique<ShowEvent>("processes", true));
            EventManager::getInstance().processEvents(ivec2(0, 0), ivec2(RESX - 1, RESY - 1));
        }

        shot.clear();
        LayoutManager::getInstance().render(shot, ivec2(0, 0), ivec2(RESX - 1, RESY - 1));
        renderFooterLiveStats(shot);
        if (tab != "processes")
        {
            renderUptimeAndLoad(shot);
            renderDiskUsage(shot);
            renderMemBreakdownStrip(shot);
            renderPerCoreStrip(shot);
            if (g_sparklinesEnabled)
            {
                if (g_cpuSparkline != nullptr) g_cpuSparkline->draw(shot);
                if (g_ramSparkline != nullptr) g_ramSparkline->draw(shot);
                if (g_gpuSparkline != nullptr) g_gpuSparkline->draw(shot);
                renderSparklineLabels(shot);
            }
        }
        else if (!g_filterText.empty())
        {
            const vec3 labelColor(0.871f, 1.0f, 0.608f);
            const vec3 textColor(1.0f, 1.0f, 1.0f);
            Font::drawText(shot, "filter: ", ivec2(24, 44), labelColor);
            Font::drawText(shot, g_filterText, ivec2(120, 44), textColor);
        }
        SDL_Surface *out = SDL_CreateSurface(RESX, RESY, SDL_PIXELFORMAT_RGBA32);
        if (out == nullptr)
        {
            std::cerr << "Failed to create screenshot surface: " << SDL_GetError() << '\n';
            SDL_Quit();
            return -3;
        }
        shot.blitTo(out);

        // Pick the writer by extension so the same flag produces what the
        // README expects (PNG hero images) without forcing callers to run an
        // external converter step. Falls back to BMP for any other suffix.
        auto endsWith = [](const std::string &s, const std::string &suffix)
        {
            return s.size() >= suffix.size()
                   && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        };
        bool saved = false;
        if (endsWith(screenshotPath, ".png") || endsWith(screenshotPath, ".PNG"))
        {
            saved = IMG_SavePNG(out, screenshotPath.c_str());
        }
        else
        {
            saved = SDL_SaveBMP(out, screenshotPath.c_str());
        }
        if (!saved)
        {
            std::cerr << "Failed to save screenshot: " << SDL_GetError() << '\n';
        }
        else
        {
            std::cout << "Saved screenshot to " << screenshotPath << '\n';
        }
        SDL_DestroySurface(out);
        Font::shutdown();
        SDL_Quit();
        return 0;
    }

    SDL_Window *window = SDL_CreateWindow("CoreMetrics", RESX, RESY, SDL_WINDOW_RESIZABLE);
    if (window == nullptr)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return -2;
    }
    SDL_SetWindowMinimumSize(window, RESX / 2, RESY / 2);
    SDL_SetWindowMaximumSize(window, RESX * 3, RESY * 3);

    SDL_Surface *iconSurface = IMG_Load(AssetPath::resolve("assets/logo.png").c_str());
    if (iconSurface != nullptr)
    {
        SDL_SetWindowIcon(window, iconSurface);
        SDL_DestroySurface(iconSurface);
    }

    Screen screen(RESX, RESY);

    buildScene();
    cacheElementPointers();
    if (g_sparklinesEnabled)
    {
        buildSparklines();
    }
    pollMetrics();

    Uint64 lastPoll = SDL_GetTicks();
    Uint64 startTicks = SDL_GetTicks();
    Uint64 durationMs = (durationSeconds > 0.0)
                            ? static_cast<Uint64>(durationSeconds * 1000.0)
                            : 0;
    SDL_Event event;
    bool end = false;

    while (!end)
    {
        if (g_shutdownRequested.load())
        {
            end = true;
            break;
        }
        if (durationMs != 0 && SDL_GetTicks() - startTicks >= durationMs)
        {
            end = true;
            break;
        }
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            {
                end = true;
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                float clickX = 0.0f;
                float clickY = 0.0f;
                SDL_GetMouseState(&clickX, &clickY);
                int winW = 0;
                int winH = 0;
                SDL_GetWindowSize(window, &winW, &winH);
                if (winW <= 0 || winH <= 0)
                {
                    winW = RESX;
                    winH = RESY;
                }

                float srcAspect = static_cast<float>(RESX) / static_cast<float>(RESY);
                float dstAspect = static_cast<float>(winW) / static_cast<float>(winH);
                int viewW = winW;
                int viewH = winH;
                int viewX = 0;
                int viewY = 0;
                if (dstAspect > srcAspect)
                {
                    viewH = winH;
                    viewW = static_cast<int>(static_cast<float>(winH) * srcAspect);
                    viewX = (winW - viewW) / 2;
                }
                else
                {
                    viewW = winW;
                    viewH = static_cast<int>(static_cast<float>(winW) / srcAspect);
                    viewY = (winH - viewH) / 2;
                }

                float relX = clickX - static_cast<float>(viewX);
                float relY = clickY - static_cast<float>(viewY);
                if (viewW <= 0 || viewH <= 0)
                {
                    break;
                }
                int mx = static_cast<int>(relX * static_cast<float>(RESX) / static_cast<float>(viewW));
                int my = static_cast<int>(relY * static_cast<float>(RESY) / static_cast<float>(viewH));
                if (mx < 0 || my < 0 || mx >= RESX || my >= RESY)
                {
                    break;
                }

                if (mx >= g_exitBtnMin.x && mx <= g_exitBtnMax.x
                    && my >= g_exitBtnMin.y && my <= g_exitBtnMax.y)
                {
                    end = true;
                }
                else if (mx >= g_muteBtnMin.x && mx <= g_muteBtnMax.x
                         && my >= g_muteBtnMin.y && my <= g_muteBtnMax.y)
                {
                    g_alarmEnabled = !g_alarmEnabled;
                    if (g_muteLabel != nullptr)
                    {
                        g_muteLabel->setText(g_alarmEnabled ? "SOUND ON" : "SOUND OFF");
                    }
                }
                else
                {
                    bool headerHit = false;
                    for (int c = 0; c < 5; ++c)
                    {
                        if (mx >= g_headerColMin[c].x && mx <= g_headerColMax[c].x
                            && my >= g_headerColMin[c].y && my <= g_headerColMax[c].y)
                        {
                            SortColumn clicked = static_cast<SortColumn>(c);
                            if (g_sortColumn == clicked)
                            {
                                g_sortAscending = !g_sortAscending;
                            }
                            else
                            {
                                g_sortColumn = clicked;
                                g_sortAscending = false;
                            }
                            if (g_headerRow != nullptr)
                            {
                                const char *arrow = g_sortAscending ? " ^" : " v";
                                std::vector<std::string> hdr = {"PID", "NAME", "CPU%", "MEM%", "DISK I/O"};
                                hdr[clicked] += arrow;
                                g_headerRow->setCells(hdr);
                            }
                            headerHit = true;
                            break;
                        }
                    }
                    if (!headerHit)
                    {
                        // If the click landed on a Processes data row, take
                        // the click as a row selection instead of letting
                        // the EventManager route it to the layout tree. The
                        // row widget itself does not own click handling, so
                        // there is nothing else competing for this region.
                        bool consumedAsRowSelect = false;
                        if (processesTabActive()
                            && mx >= PROCESSES_ROW_X0 && mx <= PROCESSES_ROW_X1
                            && my >= PROCESSES_FIRST_ROW_Y
                            && my < PROCESSES_FIRST_ROW_Y + PROCESSES_VISIBLE_ROWS * PROCESS_ROW_HEIGHT)
                        {
                            int row = (my - PROCESSES_FIRST_ROW_Y) / PROCESS_ROW_HEIGHT;
                            if (row >= 0 && row < static_cast<int>(g_visiblePids.size()))
                            {
                                g_selectedRowIndex = row;
                                g_selectedPid = g_visiblePids[row];
                                consumedAsRowSelect = true;
                            }
                        }
                        if (!consumedAsRowSelect)
                        {
                            EventManager::getInstance().pushEvent(std::make_unique<ClickEvent>(mx, my));
                        }
                    }
                }
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
            {
                if (g_filterInputActive && processesTabActive())
                {
                    // SDL hands us UTF-8 fragments; append straight through.
                    // The downstream filter does case-insensitive ASCII
                    // matching so non-ASCII bytes just pass through and
                    // narrow the match the same as any other typed char.
                    g_filterText += event.text.text;
                }
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                SDL_Keycode key = event.key.key;

                // Esc and N close the signal menu without sending; if no
                // menu is open Esc clears the selection (the htop default).
                if (key == SDLK_ESCAPE)
                {
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
                    break;
                }

                // Backspace in filter input mode edits the filter; outside
                // input mode it does nothing (no other use).
                if (g_filterInputActive && key == SDLK_BACKSPACE)
                {
                    if (!g_filterText.empty())
                    {
                        g_filterText.pop_back();
                    }
                    break;
                }

                // Enter commits the filter and exits input mode; the
                // filter stays applied and Up/Down + 'k' resume working.
                if (g_filterInputActive && (key == SDLK_RETURN || key == SDLK_KP_ENTER))
                {
                    g_filterInputActive = false;
                    SDL_StopTextInput(window);
                    break;
                }

                // While the user is editing the filter, do NOT let other
                // bindings (Up/Down/k/1-6) fire underneath; that would be
                // surprising and the SDL_StartTextInput path may have
                // already enqueued the keystroke as text.
                if (g_filterInputActive)
                {
                    break;
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
                    break;
                }

                // No menu open: arrow keys move row selection; 'k' opens
                // the menu when a row is selected and the Processes tab
                // is active.
                if (!processesTabActive())
                {
                    break;
                }
                if (key == SDLK_UP || key == SDLK_DOWN)
                {
                    if (g_visiblePids.empty())
                    {
                        break;
                    }
                    int next = g_selectedRowIndex;
                    if (next < 0)
                    {
                        next = 0;
                    }
                    else if (key == SDLK_UP)
                    {
                        next = (next > 0) ? next - 1 : 0;
                    }
                    else
                    {
                        int max = static_cast<int>(g_visiblePids.size()) - 1;
                        next = (next < max) ? next + 1 : max;
                    }
                    g_selectedRowIndex = next;
                    g_selectedPid = g_visiblePids[next];
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
                break;
            }
            }
        }

        EventManager::getInstance().processEvents(ivec2(0, 0), ivec2(RESX - 1, RESY - 1));

        Uint64 now = SDL_GetTicks();
        if (now - lastPoll >= POLL_INTERVAL_MS)
        {
            pollMetrics();
            lastPoll = now;
        }

        screen.clear();
        LayoutManager::getInstance().render(screen, ivec2(0, 0), ivec2(RESX - 1, RESY - 1));
        {
            // Per-core strip and sparklines only paint while the System tab
            // is the active layout. On Processes, the rows occupy the same
            // pixel range, so painting either would overdraw the table.
            Tree<Layout> *systemNode = EventManager::findLayoutByName(
                LayoutManager::getInstance().getRoot(), "system");
            if (systemNode != nullptr && systemNode->getData().isActive())
            {
                renderUptimeAndLoad(screen);
                renderDiskUsage(screen);
                renderMemBreakdownStrip(screen);
                renderPerCoreStrip(screen);
                if (g_sparklinesEnabled)
                {
                    if (g_cpuSparkline != nullptr) g_cpuSparkline->draw(screen);
                    if (g_ramSparkline != nullptr) g_ramSparkline->draw(screen);
                    if (g_gpuSparkline != nullptr) g_gpuSparkline->draw(screen);
                    renderSparklineLabels(screen);
                }
            }
        }
        renderFooterLiveStats(screen);

        // Process-kill overlays: selected-row highlight + signal menu + the
        // brief status flash after a send. Painted over the layout tree so
        // they sit on top of the Processes table without modifying the Row
        // widget itself.
        if (processesTabActive() && (g_filterInputActive || !g_filterText.empty()))
        {
            // Filter input strip at the top of the Processes tab, above
            // the header row. Two-tone: accent label, white query text,
            // a blinking cursor when input is active.
            const vec3 labelColor(0.871f, 1.0f, 0.608f);
            const vec3 textColor(1.0f, 1.0f, 1.0f);
            const vec3 hintColor(0.55f, 0.55f, 0.55f);
            std::string prefix = g_filterInputActive ? "filter> " : "filter: ";
            std::string body = g_filterText;
            if (g_filterInputActive && ((SDL_GetTicks() / 400) % 2) == 0)
            {
                body += "_";
            }
            Font::drawText(screen, prefix, ivec2(24, 44), labelColor);
            Font::drawText(screen, body, ivec2(120, 44), textColor);
            if (!g_filterInputActive && !g_filterText.empty())
            {
                Font::drawText(screen, "Esc clears", ivec2(800, 44), hintColor);
            }
        }

        if (processesTabActive() && g_selectedRowIndex >= 0
            && g_selectedRowIndex < PROCESSES_VISIBLE_ROWS)
        {
            int y0 = PROCESSES_FIRST_ROW_Y + g_selectedRowIndex * PROCESS_ROW_HEIGHT;
            int y1 = y0 + PROCESS_ROW_HEIGHT - 1;
            // Thin accent strip on the left edge of the row. Cheap to draw,
            // does not overdraw the text in the row.
            screen.drawBox(ivec2(PROCESSES_ROW_X0 - 6, y0),
                           ivec2(PROCESSES_ROW_X0 - 2, y1),
                           COLOR_ACCENT_GREEN);
        }

        if (g_signalMenuVisible)
        {
            // Centered panel. The overlay is small and the message is the
            // information that matters; no decorative chrome beyond a 1px
            // border in the accent color so it reads as an actionable
            // foreground element.
            const int panelX0 = 200;
            const int panelY0 = 200;
            const int panelX1 = 760;
            const int panelY1 = 320;
            const vec3 panelBg(0.08f, 0.08f, 0.08f);
            const vec3 panelBorder(0.871f, 1.0f, 0.608f);
            screen.drawBox(ivec2(panelX0, panelY0), ivec2(panelX1, panelY1), panelBg);
            // Border. 4 thin boxes is cheaper than a stroked rectangle.
            screen.drawBox(ivec2(panelX0, panelY0), ivec2(panelX1, panelY0 + 1), panelBorder);
            screen.drawBox(ivec2(panelX0, panelY1 - 1), ivec2(panelX1, panelY1), panelBorder);
            screen.drawBox(ivec2(panelX0, panelY0), ivec2(panelX0 + 1, panelY1), panelBorder);
            screen.drawBox(ivec2(panelX1 - 1, panelY0), ivec2(panelX1, panelY1), panelBorder);

            const vec3 textColor(1.0f, 1.0f, 1.0f);
            const vec3 hintColor(0.6f, 0.6f, 0.6f);
            if (g_signalMenuPickedIndex < 0)
            {
                Font::drawText(screen, "Send signal to pid " + std::to_string(g_selectedPid),
                               ivec2(panelX0 + 24, panelY0 + 18), textColor);
                Font::drawText(screen, "1 TERM   2 KILL   3 INT   4 HUP   5 STOP   6 CONT",
                               ivec2(panelX0 + 24, panelY0 + 50), COLOR_ACCENT_GREEN);
                Font::drawText(screen, "Esc cancels",
                               ivec2(panelX0 + 24, panelY1 - 30), hintColor);
            }
            else
            {
                SignalUtils::Signal sig =
                    static_cast<SignalUtils::Signal>(g_signalMenuPickedIndex);
                std::string prompt = std::string("Send ")
                                     + SignalUtils::name(sig)
                                     + " to pid " + std::to_string(g_selectedPid) + "?";
                Font::drawText(screen, prompt,
                               ivec2(panelX0 + 24, panelY0 + 18), textColor);
                Font::drawText(screen, "Y / Enter = send     N / Esc = cancel",
                               ivec2(panelX0 + 24, panelY0 + 50), COLOR_ACCENT_GREEN);
            }
        }

        if (!g_statusFlash.empty() && SDL_GetTicks() < g_statusFlashExpiryMs)
        {
            // Paint the flash over the right end of the footer status strip,
            // before the EXIT button. The accent color is reused so the flash
            // reads as part of the same status row, not a foreign popup.
            Font::drawText(screen, g_statusFlash, ivec2(540, 492), COLOR_ACCENT_GREEN);
        }
        else if (!g_statusFlash.empty())
        {
            g_statusFlash.clear();
        }

        screen.blitTo(SDL_GetWindowSurface(window));
        SDL_UpdateWindowSurface(window);
    }

    destroySparklines();
    SoundPlayer::getInstance().shutdown();
    Font::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
