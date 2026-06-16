#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#endif
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
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
#include "Thresholds.hpp"
#include "Theme.hpp"
#include "AssetPath.hpp"
#include "SignalUtils.hpp"
#include "KillLog.hpp"
#include "Exporter.hpp"
#include "Settings.hpp"

constexpr int RESX = 960;
constexpr int RESY = 540;
constexpr int PROCESS_ROW_HEIGHT = 20;
// Default poll interval is 500ms; overridable at startup via the
// --poll-ms <N> CLI flag (clamped to [100, 10000] so the UI cannot
// be told to repaint faster than the OS can serve samples or so slow
// the chrome looks frozen).
static Uint64 g_pollIntervalMs = 500;
constexpr Uint64 POLL_INTERVAL_MIN_MS = 100;
constexpr Uint64 POLL_INTERVAL_MAX_MS = 10000;

constexpr float ALARM_THRESHOLD = 80.0f;
static const std::string ALARM_SOUND_PATH = AssetPath::resolve("assets/click.wav");

static const vec3 COLOR_ACCENT_GREEN(0.871f, 1.0f, 0.608f);

static Bar *g_cpuBar = nullptr;
static Bar *g_ramBar = nullptr;
static Bar *g_gpuBar = nullptr;
static Label *g_cpuReadout = nullptr;
static Label *g_ramReadout = nullptr;
static Label *g_gpuReadout = nullptr;
static Label *g_muteLabel = nullptr;
static Label *g_pollLabel = nullptr;
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
static std::unique_ptr<Sparkline> g_cpuSparkline;
static std::unique_ptr<Sparkline> g_ramSparkline;
static std::unique_ptr<Sparkline> g_gpuSparkline;
// Network throughput history: two polylines overlaid in the same rect
// directly below the GPU strip. rx in accent green, tx in orange so a
// reviewer can read incoming vs outgoing traffic at a glance without a
// legend. y-range fixed at 0..2048 KB/s so the chart stays a stable
// comparison surface across ticks (samples are clamped, not stretched).
static std::unique_ptr<Sparkline> g_netRxSparkline;
static std::unique_ptr<Sparkline> g_netTxSparkline;
constexpr float NET_SPARKLINE_MAX_KBPS = 2048.0f;

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
// First-row index in the fetched process list. PgDown/PgUp shift the
// visible window by PROCESSES_VISIBLE_ROWS; up/down arrow at the edge
// of the visible window scrolls by one. Stays clamped to
// [0, max(0, procCount - dataRowCount)] so the bottom row of data
// fills the last visible slot even on overscroll.
static std::size_t g_processScrollOffset = 0;
// Snapshot of the most recent procs.size() after filter+truncate, set
// by pollMetrics for the key handler so PgDown / arrow keys can clamp
// against the actual table length without recomputing.
static std::size_t g_processVisibleCount = 0;
// Per-visible-row glyph overlay state for tree mode. When non-empty,
// the prefix string contains the indent spaces + '+ ' or '- ' that
// should be re-painted in color over the white text the Row already
// drew. Same length as g_visiblePids; empty string means no glyph
// (leaf row or flat mode). Parallel bool flags whether the row is
// collapsed (true => grey '+') or expanded (false => green '-').
static std::vector<std::string> g_rowGlyphPrefix;
static std::vector<bool> g_rowGlyphCollapsed;

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
// Per-pid collapse state for tree mode. Pids in this set hide their
// descendants in the flattened row list; pressing 'space' on a row
// toggles membership. Initial state: empty (everything expanded).
// Kept across tree-mode toggles so the user's layout sticks.
static std::unordered_set<int> g_collapsedPids;

// Help overlay: a translucent-feel dark panel listing every hotkey,
// toggled by '?' and dismissed with '?' or Esc. Painted last in the
// render pass so it sits on top of every tab and every other overlay.
static bool g_helpOverlayVisible = false;

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
// Full row width. The strip sits directly under the GPU bar and reads
// as a per-core continuation without a separate label; the earlier
// 'cores' label at x=24 collided with the 'CPU history' sparkline
// label at y=230 (12-px gap, ~16-px glyph height).
// Strip starts at x=84 so a "cores" label fits at x=24, mirroring the
// CPU / RAM / GPU column above. Label color hidden when --sparklines
// is on (the CPU history label at y=230 would collide visually if both
// rendered at full opacity).
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
// Aggregate disk I/O rate summed across the top-N processes the
// backend returned. Shown on the System tab as 'I/O R X.X MB/s
// W Y.Y MB/s' so reviewers see system-wide disk pressure without
// scanning the per-process table.
static unsigned long long g_aggregateDiskReadKbPerSec = 0;
static unsigned long long g_aggregateDiskWriteKbPerSec = 0;
// Aggregate network rx/tx sampled every poll. Loopback excluded.
static NetIo g_netIo{0, 0};
constexpr int MEMSEG_X0 = 84;
constexpr int MEMSEG_X1 = 864;
// Slim strip (164..172) so the breakdown reads as a continuation of
// the RAM bar above, not a second RAM bar stacked beneath. The earlier
// 14px height made the two bars feel like duplicate metrics; 8px keeps
// the segment colors readable while letting the eye treat the strip
// as a thin annotation rather than its own row.
// Memory breakdown strip thinned to a 4px sliver (was 8px) and pinned
// directly under the RAM bar so the System tab reads as one bar with a
// thin annotation instead of two stacked bars.
constexpr int MEMSEG_Y0 = 162;
constexpr int MEMSEG_Y1 = 166;

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

// Repaint the Processes-tab header row so the active sort column carries
// a trailing direction glyph (' ^' ascending, ' v' descending) and the
// other four columns stay clean. Called on initial scene build and on
// every header click so the indicator is sticky across polls, not only
// visible immediately after the user clicked.
static void updateHeaderSortGlyph()
{
    if (g_headerRow == nullptr)
    {
        return;
    }
    const char *arrow = g_sortAscending ? " ^" : " v";
    std::vector<std::string> hdr = {"PID", "NAME", "CPU%", "MEM%", "DISK I/O"};
    hdr[static_cast<int>(g_sortColumn)] += arrow;
    g_headerRow->setCells(hdr);
}

static void buildScene()
{
    LayoutManager &manager = LayoutManager::getInstance();
    GUIFile g;
    g.readFile(AssetPath::resolve("base.xml"), manager);

    g_muteBtnMin = ivec2(812, 8);
    g_muteBtnMax = ivec2(952, 40);

    g_exitBtnMin = ivec2(820, 478);
    g_exitBtnMax = ivec2(944, 520);

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
        // The percent readouts are the heaviest piece of information on the
        // System dashboard; UI audit wave 5 flagged them as visually equal to
        // the static "CPU"/"RAM"/"GPU" stems. Tagging them bold double-blits
        // the glyph surface (see Font::drawTextBold) so the numbers carry
        // more weight without bundling a second TTF.
        if (g_cpuReadout != nullptr)
        {
            g_cpuReadout->setBold(true);
        }
        if (g_ramReadout != nullptr)
        {
            g_ramReadout->setBold(true);
        }
        if (g_gpuReadout != nullptr)
        {
            g_gpuReadout->setBold(true);
        }
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
        // Seed the header with the current sort glyph so the indicator is
        // visible from the first frame, not only after the user clicks a
        // header. Keeps the screenshot/headless renders honest too.
        updateHeaderSortGlyph();
    }

    // Find the footer 'POLL_PLACEHOLDER' label so the displayed poll
    // interval can track the --poll-ms override at runtime. Walk every
    // layout in the tree because the footer labels could end up at any
    // depth depending on base.xml structure.
    std::vector<Tree<Layout> *> stack;
    stack.push_back(&root);
    while (!stack.empty() && g_pollLabel == nullptr)
    {
        Tree<Layout> *node = stack.back();
        stack.pop_back();
        for (const auto &element : node->getData().elements)
        {
            Label *lbl = dynamic_cast<Label *>(element.get());
            if (lbl != nullptr && lbl->getText() == "POLL_PLACEHOLDER")
            {
                g_pollLabel = lbl;
                break;
            }
        }
        for (auto &child : node->getChildren())
        {
            stack.push_back(child.get());
        }
    }
    if (g_pollLabel != nullptr)
    {
        g_pollLabel->setText("poll " + std::to_string(g_pollIntervalMs) + "ms");
    }
}

static void buildSparklines()
{
    // Stacked sparklines in the System tab's empty lower half. Each row spans
    // the same horizontal range as the bars above (x in [24, 864]) and
    // matches the same accent color, so the polyline reads as a continuation
    // of its bar. Heights are 56px / row with 12px gaps.
    const vec3 accent = Theme::accentGreen();
    // Sparkline stack: 4 charts at 40px each with a 14px label band above
    // each chart. Labels live at chart-top minus 12px so they sit cleanly
    // above the polyline instead of being overwritten by the fill. Tick
    // labels ("100" / "0") render at maxPos.x + 4 in the 80px right
    // gutter (chart maxPos.x = 864, screen width = 960). 0 tick now sits
    // 4px BELOW the chart (maxPos.y + 4) so it never sits inside the
    // polyline fill area.
    g_cpuSparkline = std::make_unique<Sparkline>(
        ivec2(24, 246), ivec2(864, 286), accent, 0.0f, 100.0f, SPARKLINE_CAPACITY);
    g_cpuSparkline->setThresholdMode(true);
    g_ramSparkline = std::make_unique<Sparkline>(
        ivec2(24, 312), ivec2(864, 352), accent, 0.0f, 100.0f, SPARKLINE_CAPACITY);
    g_ramSparkline->setThresholdMode(true);
    g_gpuSparkline = std::make_unique<Sparkline>(
        ivec2(24, 378), ivec2(864, 418), accent, 0.0f, 100.0f, SPARKLINE_CAPACITY);
    g_gpuSparkline->setThresholdMode(true);
    // tx (orange) is constructed first so it renders behind the rx
    // (green) line in the draw pass; incoming traffic reads as more
    // important. NET strip ends at y=474 leaving 8px clear of the
    // y=482..516 EXIT button + footer chrome.
    const vec3 netTx(0.95f, 0.65f, 0.30f);
    const vec3 netRx(0.5f, 0.85f, 0.5f);
    g_netTxSparkline = std::make_unique<Sparkline>(
        ivec2(24, 444), ivec2(864, 474), netTx, 0.0f, NET_SPARKLINE_MAX_KBPS, SPARKLINE_CAPACITY);
    g_netRxSparkline = std::make_unique<Sparkline>(
        ivec2(24, 444), ivec2(864, 474), netRx, 0.0f, NET_SPARKLINE_MAX_KBPS, SPARKLINE_CAPACITY);
}

static void destroySparklines()
{
    g_cpuSparkline.reset();
    g_ramSparkline.reset();
    g_gpuSparkline.reset();
    g_netRxSparkline.reset();
    g_netTxSparkline.reset();
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
    const vec3 dimColor = Theme::textDim();
    Font::drawText(dest, formatUptimeString(g_uptimeSeconds), ivec2(24, 44), dimColor);
    // Uptime + Load + Disk all share the y=44 baseline so the status
    // row reads as a single line instead of a staircase. The earlier
    // y=56 placement for Load predated the DISK strip and left Load
    // ~12 px below Up, which was visible as a misaligned middle field.
    Font::drawText(dest, formatLoadAverages(), ivec2(220, 44), dimColor);
}

static std::string formatRate(unsigned long long kbPerSec)
{
    if (kbPerSec >= 1024)
    {
        std::ostringstream oss;
        oss.precision(1);
        oss << std::fixed << (static_cast<double>(kbPerSec) / 1024.0) << " MB/s";
        return oss.str();
    }
    return std::to_string(kbPerSec) + " KB/s";
}

static void renderAggregateDiskIo(Screen &dest)
{
    // Disk I/O is now sampled but rendered as part of renderNetIo to
    // produce a single right-aligned 'disk + net' string in the footer
    // chrome. This function is left as a no-op so the existing call
    // sites do not need to change; consolidating both rates into one
    // line keeps the footer compact and predictable.
    (void)dest;
}

static void renderNetIo(Screen &dest)
{
    bool anyDisk = (g_aggregateDiskReadKbPerSec != 0
                    || g_aggregateDiskWriteKbPerSec != 0);
    bool anyNet = (g_netIo.rxKbPerSec != 0 || g_netIo.txKbPerSec != 0);
    if (!anyDisk && !anyNet)
    {
        return;
    }
    const vec3 dim = Theme::textDim();
    // Compact units (K/M/G) keep the worst-case string bounded: even a
    // 10 GB/s host renders as '10.0G' (5 chars) rather than '10240.0M'
    // (8 chars). Caps the right edge of the footer well before x=810
    // (the EXIT button's left edge).
    auto compact = [](unsigned long long kbPerSec) -> std::string {
        if (kbPerSec >= 1024ULL * 1024ULL)
        {
            std::ostringstream oss;
            oss.precision(1);
            oss << std::fixed
                << (static_cast<double>(kbPerSec) / (1024.0 * 1024.0))
                << "G";
            return oss.str();
        }
        if (kbPerSec >= 1024)
        {
            std::ostringstream oss;
            oss.precision(1);
            oss << std::fixed << (static_cast<double>(kbPerSec) / 1024.0) << "M";
            return oss.str();
        }
        return std::to_string(kbPerSec) + "K";
    };
    std::string text;
    if (anyDisk)
    {
        text = "DISK " + compact(g_aggregateDiskReadKbPerSec)
               + "/" + compact(g_aggregateDiskWriteKbPerSec);
    }
    if (anyNet)
    {
        if (!text.empty()) text += "  ";
        text += "NET " + compact(g_netIo.rxKbPerSec)
                + "/" + compact(g_netIo.txKbPerSec);
    }
    // Worst-case string at G-scale is roughly 34 chars; at 10px/glyph
    // that lands the right edge at ~x=800, just left of the EXIT
    // button at x=820. Hard-clip the string at 36 chars so a future
    // unit-format change cannot blow through the budget.
    if (text.size() > 36)
    {
        text = text.substr(0, 36);
    }
    Font::drawText(dest, text, ivec2(460, 492), dim);
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

    vec3 color = Theme::textDim();
    if (pct >= Thresholds::RED_PCT)
    {
        color = Thresholds::colorRed();
    }
    else if (pct >= Thresholds::YELLOW_PCT)
    {
        color = Thresholds::colorYellow();
    }

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

    const vec3 colActive = Theme::accentRed(); // red: in-use by processes
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
    // Left-edge label sits in the same x=24 column as the CPU/RAM/GPU
    // labels above. Hidden when --sparklines is enabled because the
    // CPU history label at y=230 would crowd it.
    if (!g_sparklinesEnabled)
    {
        const vec3 dim = Theme::textDim();
        Font::drawText(dest, "cores", ivec2(24, 218), dim);
    }
    const std::size_t cores = g_perCoreCpu.size();
    const int totalGap = PERCORE_GAP * static_cast<int>(cores - 1);
    const int slot = (PERCORE_X1 - PERCORE_X0 - totalGap) / static_cast<int>(cores);
    if (slot <= 0)
    {
        return;
    }
    const vec3 bg = Theme::panelBg();

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
        vec3 color = Thresholds::colorForRatio(ratio);
        dest.drawBox(ivec2(x0, PERCORE_Y0), ivec2(x0 + fillW, PERCORE_Y1), color);
    }
}

// Live footer string replacing the v0.1.x static "alarm threshold 80%"
// label. Same color and y-baseline as the other footer chrome
// (coremetrics / v0.2.0 / poll 500ms) so it reads as a continuation.
// Latest process count is set by pollMetrics().
static std::size_t g_lastProcCount = 0;
// Latest summed CPU% / MEM% across the topProcesses sample. Maintained
// alongside g_lastProcCount in pollMetrics() and read by the Processes
// tab summary strip so a reviewer can eyeball aggregate pressure without
// summing the visible table.
static float g_sumCpuPct = 0.0f;
static float g_sumMemPct = 0.0f;

static void renderFooterLiveStats(Screen &dest)
{
    // procs N at x=370. The DISK+NET text begins at x=460. To survive
    // a 4-digit proc count (10 chars * ~10px = 100px ending at x=470),
    // truncate the count to a max width budget rather than printing
    // arbitrary integers. Real-world process counts above ~9999 are
    // already pathological on a personal machine.
    const vec3 dim(0.5f, 0.5f, 0.5f);
    std::size_t n = g_lastProcCount;
    if (n > 9999) n = 9999;
    std::string text = std::to_string(n) + " procs";
    Font::drawText(dest, text, ivec2(370, 492), dim);
}

// Processes-tab aggregate strip painted above the column headers, below
// the tab strip. Reads the cached g_lastProcCount / g_sumCpuPct /
// g_sumMemPct that pollMetrics() refreshes each tick so the strip stays
// in sync with the table without a second pass over the process list.
// Total count uses accent green; the CPU/MEM sums use the dim grey the
// rest of the footer chrome uses so the eye lands on the count first.
static void renderProcessesSummary(Screen &dest)
{
    // Strip lives at y=448..464, BELOW the 15 visible data rows
    // (y=100..400) and ABOVE the footer chrome (y=476..520). Earlier
    // versions sat the strip at y=40..60 which physically collided
    // with the y=8..40 tab buttons defined in base.xml; glyph
    // descenders bled into the button outlines.
    const vec3 stripBg(0.08f, 0.08f, 0.08f);
    dest.drawBox(ivec2(24, 448), ivec2(936, 466), stripBg);

    const vec3 dim(0.65f, 0.65f, 0.65f);
    std::size_t n = g_lastProcCount;
    if (n > 9999) n = 9999;
    std::string totalText = "Total: " + std::to_string(n) + " procs";
    std::string cpuText = "Sum CPU: " + formatPct(g_sumCpuPct) + "%";
    std::string memText = "Sum MEM: " + formatPct(g_sumMemPct) + "%";

    Font::drawText(dest, totalText, ivec2(32, 452), COLOR_ACCENT_GREEN);
    Font::drawText(dest, cpuText, ivec2(280, 452), dim);
    Font::drawText(dest, memText, ivec2(520, 452), dim);

    // Scroll position indicator. Only shown when the table is longer
    // than the visible window so a user with 22 processes does not see
    // a meaningless "1-15 / 22" on a static page; the moment it appears
    // it signals "PgDown/Down arrow to scroll".
    std::size_t windowSize = PROCESSES_VISIBLE_ROWS;
    if (g_processVisibleCount > windowSize)
    {
        std::size_t firstRow = g_processScrollOffset + 1;
        std::size_t lastRow = g_processScrollOffset + windowSize;
        if (lastRow > g_processVisibleCount)
        {
            lastRow = g_processVisibleCount;
        }
        std::string scrollText = std::to_string(firstRow) + ".."
                                 + std::to_string(lastRow) + " / "
                                 + std::to_string(g_processVisibleCount);
        Font::drawText(dest, scrollText, ivec2(760, 452), COLOR_ACCENT_GREEN);
    }
}

// Keyboard shortcuts overlay. Painted last so it sits on top of every
// tab and every other overlay. The panel is a near-black box with the
// accent green used for the key glyph column and a dim grey for each
// shortcut's description, mirroring the palette used by the signal menu
// so the help reads as the same surface family. Lines are kept terse
// (~10 entries) so the panel never overflows its 720x380 footprint.
static void renderHelpOverlay(Screen &dest)
{
    const int panelX0 = 120;
    const int panelY0 = 80;
    const int panelX1 = 840;
    const int panelY1 = 460;
    const vec3 panelBg = Theme::bgDark();
    const vec3 panelBorder = Theme::accentGreen();
    dest.drawBox(ivec2(panelX0, panelY0), ivec2(panelX1, panelY1), panelBg);
    // Four thin boxes draw the 1px border; cheaper than a stroked rect
    // and matches the signal menu treatment.
    dest.drawBox(ivec2(panelX0, panelY0), ivec2(panelX1, panelY0 + 1), panelBorder);
    dest.drawBox(ivec2(panelX0, panelY1 - 1), ivec2(panelX1, panelY1), panelBorder);
    dest.drawBox(ivec2(panelX0, panelY0), ivec2(panelX0 + 1, panelY1), panelBorder);
    dest.drawBox(ivec2(panelX1 - 1, panelY0), ivec2(panelX1, panelY1), panelBorder);

    const vec3 titleColor(1.0f, 1.0f, 1.0f);
    const vec3 keyColor = COLOR_ACCENT_GREEN;
    const vec3 descColor(0.65f, 0.65f, 0.65f);
    const vec3 hintColor(0.5f, 0.5f, 0.5f);

    Font::drawText(dest, "Keyboard shortcuts",
                   ivec2(panelX0 + 24, panelY0 + 18), titleColor);

    // Two-column layout: key glyph at x+24, description at x+160.
    // Row pitch 28px gives ~10 lines comfortably inside the 380px panel.
    const int keyX = panelX0 + 24;
    const int descX = panelX0 + 160;
    int row = panelY0 + 60;
    constexpr int rowStep = 28;

    Font::drawText(dest, "?", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "toggle this help", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "1..N", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "switch tab", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "/", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "filter processes by name", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "t", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "toggle tree mode", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "up / down", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "row select (auto-scroll at edge)", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "pgup / pgdn", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "scroll process list by a page", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "home / end", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "jump to top / bottom of list", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "wheel", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "scroll process list (3 rows/tick)", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "k", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "open signal / kill menu", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "y / enter", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "confirm action", ivec2(descX, row), descColor);
    row += rowStep;

    Font::drawText(dest, "esc", ivec2(keyX, row), keyColor);
    Font::drawText(dest, "cancel / clear filter / dismiss", ivec2(descX, row), descColor);

    Font::drawText(dest, "press ? or esc to close",
                   ivec2(panelX0 + 24, panelY1 - 28), hintColor);
}

// Tiny accent labels at the left edge of each sparkline strip. Without
// them three unlabeled polylines at the bottom of the System tab read as
// abstract decoration; with them a reviewer instantly sees they're CPU
// / RAM / GPU history. y-baseline picked to sit just above each strip.
static void renderSparklineLabels(Screen &dest)
{
    const vec3 dim = Theme::textDim();
    // One label per sparkline, painted 12px above the chart's top edge
    // so the text sits in clear sky above the polyline fill instead of
    // getting overwritten on every peak. Chart rects are CPU 246..286,
    // RAM 312..352, GPU 378..418, NET 444..474.
    Font::drawText(dest, "CPU history", ivec2(24, 230), dim);
    Font::drawText(dest, "RAM history", ivec2(24, 296), dim);
    Font::drawText(dest, "GPU history", ivec2(24, 362), dim);
    Font::drawText(dest, "NET history", ivec2(24, 428), dim);
}

static void pollMetrics()
{
    float cpuPct = SystemMetrics::readCpuPercent();
    float memPct = SystemMetrics::readMemPercent();
    float gpuPct = SystemMetrics::readGpuPercent();

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
    if (g_cpuReadout != nullptr)
    {
        g_cpuReadout->setText(formatPct(cpuPct) + "%");
        g_cpuReadout->setColor(Thresholds::colorForPct(cpuPct));
    }
    if (g_ramReadout != nullptr)
    {
        g_ramReadout->setText(formatPct(memPct) + "%");
        g_ramReadout->setColor(Thresholds::colorForPct(memPct));
    }
    if (g_gpuReadout != nullptr)
    {
        g_gpuReadout->setText(formatPct(gpuPct) + "%");
        g_gpuReadout->setColor(Thresholds::colorForPct(gpuPct));
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
    // Cached for renderProcessesSummary so the strip can paint each frame
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

enum class TopSortKey
{
    Memory,
    Cpu,
    Io
};

static bool topSortByCpuDesc(const ProcessInfo &a, const ProcessInfo &b)
{
    return a.cpuPct > b.cpuPct;
}

static bool topSortByIoDesc(const ProcessInfo &a, const ProcessInfo &b)
{
    unsigned long long aIo = a.diskReadKbPerSec + a.diskWriteKbPerSec;
    unsigned long long bIo = b.diskReadKbPerSec + b.diskWriteKbPerSec;
    return aIo > bIo;
}

// Pick an ANSI 24-bit color escape for a percent value using the same
// green/yellow/red threshold palette the GUI uses (see Thresholds.hpp).
// Returns "" when colorization is off so the caller can splice it in
// unconditionally without surrounding if/else noise.
static const char *topColorForPct(float pct, bool colorize)
{
    if (!colorize)
    {
        return "";
    }
    if (pct >= 80.0f)
    {
        return "\033[31m"; // red
    }
    if (pct >= 60.0f)
    {
        return "\033[33m"; // yellow
    }
    return "\033[32m"; // green
}

// Pretty-print the top-N process table to stdout for the headless
// `--top` and `--watch` modes. Separated so both the one-shot path
// and the watch loop call the exact same formatter. Project rule
// forbids lambdas in app code, so this is a free static helper.
static void printTopProcesses(int topCount, TopSortKey sortKey, bool colorize)
{
    // Over-fetch by 4x so the secondary sort by cpu/io has enough
    // candidates to choose from when the backend ordered the list by
    // memory. topProcesses() returns by mem% desc; we re-sort here.
    std::size_t fetchN = static_cast<std::size_t>(topCount) * 4;
    if (fetchN < 50)
    {
        fetchN = 50;
    }
    std::vector<ProcessInfo> procs = SystemMetrics::topProcesses(fetchN);
    if (sortKey == TopSortKey::Cpu)
    {
        std::sort(procs.begin(), procs.end(), topSortByCpuDesc);
    }
    else if (sortKey == TopSortKey::Io)
    {
        std::sort(procs.begin(), procs.end(), topSortByIoDesc);
    }
    if (procs.size() > static_cast<std::size_t>(topCount))
    {
        procs.resize(static_cast<std::size_t>(topCount));
    }
    const char *reset = colorize ? "\033[0m" : "";
    const char *headerColor = colorize ? "\033[1m" : "";
    std::printf("%s%-7s %-32s %6s %6s %12s%s\n",
                headerColor, "PID", "NAME", "CPU%", "MEM%", "IO KB/s", reset);
    for (const auto &p : procs)
    {
        std::string name = p.name;
        if (name.size() > 32)
        {
            name.resize(32);
        }
        unsigned long long ioKbPerSec = p.diskReadKbPerSec + p.diskWriteKbPerSec;
        const char *cpuC = topColorForPct(p.cpuPct, colorize);
        const char *memC = topColorForPct(p.memPct, colorize);
        std::printf("%-7d %-32s %s%6.1f%s %s%6.1f%s %12llu\n",
                    p.pid, name.c_str(),
                    cpuC, p.cpuPct, reset,
                    memC, p.memPct, reset,
                    ioKbPerSec);
    }
    std::fflush(stdout);
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
    // Optional `--export-csv <path>` / `--export-json <path>` write one
    // sample of the live aggregates plus the top-N process list to a flat
    // file and exit. Both run before SDL_Init so they need no display.
    std::string exportCsvPath;
    std::string exportJsonPath;
    // `--top N` prints the top N processes by memory % to stdout as a
    // plain text table and exits. Lets the binary be piped into shell
    // tools without scraping a screenshot or a CSV file. N is clamped
    // to [1, 999]; default 20 if the value is missing or unparseable.
    int topCount = 0;
    // `--watch` switches --top from one-shot to live tail-style: the
    // table re-prints every poll interval, clearing the terminal first
    // so the latest snapshot replaces the previous one in place. Ctrl-C
    // exits cleanly via the existing SIGINT handler.
    bool watchMode = false;
    // `--top-sort cpu|mem|io` re-orders the printed table by the
    // chosen column. Default is mem (matches the backend's natural
    // sort order from topProcesses(N)). Unknown values fall back to mem.
    TopSortKey topSortKey = TopSortKey::Memory;
    // `--top-color auto|always|never`: colorize the --top / --watch
    // output. auto (default) checks isatty so piped output stays
    // ASCII-clean. always forces color on. never forces it off.
    int topColorMode = 0; // 0=auto, 1=always, 2=never

    // Hydrate persisted preferences before argv parsing so any CLI
    // flag the user supplies this run overrides the saved value.
    // Settings::load is a no-op on a fresh install with no config file
    // and leaves each global untouched on missing or malformed keys.
    int loadedSortColumn = static_cast<int>(g_sortColumn);
    Settings::load(g_pollIntervalMs,
                   g_sparklinesEnabled,
                   loadedSortColumn,
                   g_sortAscending,
                   g_collapsedPids);
    g_sortColumn = static_cast<SortColumn>(loadedSortColumn);

    // --help / -h print the same flag reference the manpage carries
    // and exit 0 before any other state is touched. Lets users see
    // the full CLI without needing 'man coremetrics' installed.
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            std::printf(
                "Usage: coremetrics [options]\n"
                "\n"
                "Live UI flags:\n"
                "  --sparklines              show CPU/RAM/GPU/NET history sparklines\n"
                "  --tree                    open Processes tab in parent/child tree mode\n"
                "  --filter PATTERN          seed the Processes-tab name filter\n"
                "  --poll-ms N               refresh cadence in ms (clamped 100..10000)\n"
                "  --duration SECONDS        auto-exit after N seconds (live UI)\n"
                "\n"
                "Headless modes:\n"
                "  --screenshot PATH [system|processes]\n"
                "                            render one frame to PATH and exit\n"
                "  --export-csv PATH         dump one-shot CSV and exit\n"
                "  --export-json PATH        dump one-shot JSON and exit\n"
                "  --top N                   print top-N processes to stdout and exit\n"
                "  --watch                   used with --top: re-print every poll interval\n"
                "  --top-sort cpu|mem|io     re-order --top output (default mem)\n"
                "  --top-color auto|always|never\n"
                "                            colorize --top output (default auto via isatty)\n"
                "\n"
                "Other:\n"
                "  -h, --help                print this help and exit\n"
                "\n"
                "See coremetrics(1) for the full manpage and key bindings.\n"
            );
            return 0;
        }
    }

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
        // --poll-ms <N> overrides the default 500ms refresh cadence.
        // Validation + clamp delegated to ProcessUtils so the same
        // logic gets unit coverage and negative / non-numeric inputs
        // fall back to the default instead of wrapping.
        if (std::string(argv[i]) == "--poll-ms" && i + 1 < argc)
        {
            g_pollIntervalMs = static_cast<Uint64>(
                clampPollIntervalMs(argv[i + 1], g_pollIntervalMs));
        }
        // --export-csv <path> / --export-json <path>: one-shot dump of the
        // live aggregates + top-N process list, written to `path` and then
        // the process exits 0. Lets external tools consume CoreMetrics data
        // without scraping screenshots.
        if (std::string(argv[i]) == "--export-csv" && i + 1 < argc)
        {
            exportCsvPath = argv[i + 1];
        }
        if (std::string(argv[i]) == "--export-json" && i + 1 < argc)
        {
            exportJsonPath = argv[i + 1];
        }
        if (std::string(argv[i]) == "--top")
        {
            int parsed = 20;
            if (i + 1 < argc)
            {
                parsed = std::atoi(argv[i + 1]);
                if (parsed < 1)
                {
                    parsed = 20;
                }
                if (parsed > 999)
                {
                    parsed = 999;
                }
            }
            topCount = parsed;
        }
        if (std::string(argv[i]) == "--watch")
        {
            watchMode = true;
        }
        if (std::string(argv[i]) == "--top-sort" && i + 1 < argc)
        {
            std::string key = argv[i + 1];
            if (key == "cpu")
            {
                topSortKey = TopSortKey::Cpu;
            }
            else if (key == "io")
            {
                topSortKey = TopSortKey::Io;
            }
            else
            {
                topSortKey = TopSortKey::Memory;
            }
        }
        if (std::string(argv[i]) == "--top-color" && i + 1 < argc)
        {
            std::string mode = argv[i + 1];
            if (mode == "always")
            {
                topColorMode = 1;
            }
            else if (mode == "never")
            {
                topColorMode = 2;
            }
            else
            {
                topColorMode = 0;
            }
        }
    }

    // Resolve the auto color mode by checking whether stdout is a TTY.
    // Pipes and CI logs get plain text; an interactive terminal gets
    // the threshold-colored CPU% / MEM% cells.
    bool topColorize = false;
    if (topColorMode == 1)
    {
        topColorize = true;
    }
    else if (topColorMode == 0)
    {
        topColorize = (isatty(fileno(stdout)) != 0);
    }

    // `--top` runs before SDL_Init since it never paints. Prints a
    // fixed-width text table to stdout (PID/NAME/CPU%/MEM%/IO) and
    // exits 0. Reads metrics with the same backend the live UI uses
    // so output reflects the same numbers a user would see on screen.
    if (topCount > 0)
    {
        // Install the signal handler before the loop so Ctrl-C exits
        // cleanly out of --watch instead of leaving a half-cleared
        // terminal. The same flag the GUI main loop watches.
        std::signal(SIGINT, handleShutdownSignal);
        std::signal(SIGTERM, handleShutdownSignal);
        if (!watchMode)
        {
            printTopProcesses(topCount, topSortKey, topColorize);
            return 0;
        }
        // --watch: clear the terminal with the ANSI 2J + cursor home
        // sequence then re-print. Cheap, no curses, works in any
        // terminal emulator. Uses g_pollIntervalMs (defaults 500 ms,
        // overridable via --poll-ms) so the cadence matches the GUI.
        while (!g_shutdownRequested.load())
        {
            std::printf("\033[2J\033[H");
            printTopProcesses(topCount, topSortKey, topColorize);
            SDL_Delay(static_cast<Uint32>(g_pollIntervalMs));
        }
        return 0;
    }

    // Export path runs before SDL_Init: no window, no audio, no font.
    // SystemMetrics calls are pure platform queries so they work fine
    // headless. If both flags are present we honor both.
    if (!exportCsvPath.empty() || !exportJsonPath.empty())
    {
        int exportStatus = 0;
        if (!exportCsvPath.empty())
        {
            if (!Exporter::writeCsv(exportCsvPath))
            {
                std::cerr << "Failed to write CSV export to "
                          << exportCsvPath << '\n';
                exportStatus = 1;
            }
            else
            {
                std::cout << "Wrote CSV export to " << exportCsvPath << '\n';
            }
        }
        if (!exportJsonPath.empty())
        {
            if (!Exporter::writeJson(exportJsonPath))
            {
                std::cerr << "Failed to write JSON export to "
                          << exportJsonPath << '\n';
                exportStatus = 1;
            }
            else
            {
                std::cout << "Wrote JSON export to " << exportJsonPath << '\n';
            }
        }
        return exportStatus;
    }

    std::signal(SIGINT, handleShutdownSignal);
    std::signal(SIGTERM, handleShutdownSignal);

    // Headless --screenshot does not play sound, so initializing the
    // SDL audio subsystem is wasted work and on macOS the AudioQueue
    // background thread races the screenshot teardown, producing a
    // sporadic EXC_BAD_ACCESS in pthread_mutex_lock during static
    // destructors after the PNG has already been written. Init only
    // VIDEO in the screenshot path; the live path still inits both.
    Uint32 sdlInitFlags = SDL_INIT_VIDEO;
    if (screenshotPath.empty())
    {
        sdlInitFlags |= SDL_INIT_AUDIO;
    }
    if (!SDL_Init(sdlInitFlags))
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
            renderAggregateDiskIo(shot);
            renderNetIo(shot);
            renderMemBreakdownStrip(shot);
            renderPerCoreStrip(shot);
            if (g_sparklinesEnabled)
            {
                if (g_cpuSparkline != nullptr) g_cpuSparkline->draw(shot);
                if (g_ramSparkline != nullptr) g_ramSparkline->draw(shot);
                if (g_gpuSparkline != nullptr) g_gpuSparkline->draw(shot);
                // tx (orange) first so the rx (green) line reads on top;
                // incoming traffic is the headline number for most users.
                if (g_netTxSparkline != nullptr) g_netTxSparkline->draw(shot);
                if (g_netRxSparkline != nullptr) g_netRxSparkline->draw(shot);
                renderSparklineLabels(shot);
            }
        }
        else if (!g_filterText.empty())
        {
            const vec3 labelColor = Theme::accentGreen();
            const vec3 textColor(1.0f, 1.0f, 1.0f);
            Font::drawText(shot, "filter: ", ivec2(24, 44), labelColor);
            Font::drawText(shot, g_filterText, ivec2(120, 44), textColor);
        }
        if (tab == "processes")
        {
            renderProcessesSummary(shot);
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
                            updateHeaderSortGlyph();
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
            case SDL_EVENT_MOUSE_WHEEL:
            {
                // Mouse-wheel scroll on the Processes tab moves the
                // visible window through the process list. y is the
                // vertical wheel delta (positive = scroll up). Each
                // wheel tick shifts by 3 rows so a comfortable wrist
                // motion covers most of the visible window.
                if (processesTabActive())
                {
                    constexpr std::size_t WHEEL_ROWS_PER_TICK = 3;
                    constexpr std::size_t WINDOW = PROCESSES_VISIBLE_ROWS;
                    int delta = static_cast<int>(event.wheel.y);
                    if (delta > 0)
                    {
                        std::size_t step = static_cast<std::size_t>(delta) * WHEEL_ROWS_PER_TICK;
                        if (g_processScrollOffset >= step)
                        {
                            g_processScrollOffset -= step;
                        }
                        else
                        {
                            g_processScrollOffset = 0;
                        }
                    }
                    else if (delta < 0)
                    {
                        std::size_t step = static_cast<std::size_t>(-delta) * WHEEL_ROWS_PER_TICK;
                        if (g_processVisibleCount > WINDOW)
                        {
                            std::size_t maxOffset = g_processVisibleCount - WINDOW;
                            g_processScrollOffset += step;
                            if (g_processScrollOffset > maxOffset)
                            {
                                g_processScrollOffset = maxOffset;
                            }
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

                // '?' toggles the keyboard shortcuts overlay. Lives before
                // every other handler so it works on any tab and even with
                // another overlay underneath; the help panel is read-only
                // chrome and never consumes a follow-up keystroke besides
                // its own dismiss.
                if (key == SDLK_QUESTION)
                {
                    g_helpOverlayVisible = !g_helpOverlayVisible;
                    break;
                }

                // Esc and N close the signal menu without sending; if no
                // menu is open Esc clears the selection (the htop default).
                if (key == SDLK_ESCAPE)
                {
                    if (g_helpOverlayVisible)
                    {
                        // Esc on help overlay: dismiss only. Don't fall
                        // through to clearing selection or filter so the
                        // user can dismiss without side effects.
                        g_helpOverlayVisible = false;
                        break;
                    }
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
                                // Audit trail: one line per successful kill so a
                                // user can later reconstruct which signal landed
                                // on which pid. Best-effort and silent on I/O
                                // failure so the live UI never blocks on disk.
                                KillLog::append(g_selectedPid,
                                                (g_selectedRowIndex >= 0
                                                 && g_selectedRowIndex + 1 < static_cast<int>(g_processRows.size())
                                                 && g_processRows[g_selectedRowIndex + 1] != nullptr
                                                 && g_processRows[g_selectedRowIndex + 1]->getCells().size() > 1)
                                                    ? g_processRows[g_selectedRowIndex + 1]->getCells()[1]
                                                    : std::string{},
                                                SignalUtils::name(sig));
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
                        if (next > 0)
                        {
                            next -= 1;
                        }
                        else if (g_processScrollOffset > 0)
                        {
                            // Already at top visible row; scroll the
                            // window up by one so the previous off-screen
                            // process becomes the new selection on the
                            // next poll tick.
                            g_processScrollOffset -= 1;
                        }
                    }
                    else
                    {
                        int max = static_cast<int>(g_visiblePids.size()) - 1;
                        if (next < max)
                        {
                            next += 1;
                        }
                        else if (g_processVisibleCount
                                 > g_processScrollOffset + g_visiblePids.size())
                        {
                            // Already at bottom visible row; scroll the
                            // window down by one so the next off-screen
                            // process becomes the new selection.
                            g_processScrollOffset += 1;
                        }
                    }
                    g_selectedRowIndex = next;
                    g_selectedPid = g_visiblePids[next];
                }
                else if (key == SDLK_PAGEUP)
                {
                    if (g_processScrollOffset >= PROCESSES_VISIBLE_ROWS)
                    {
                        g_processScrollOffset -= PROCESSES_VISIBLE_ROWS;
                    }
                    else
                    {
                        g_processScrollOffset = 0;
                    }
                }
                else if (key == SDLK_PAGEDOWN)
                {
                    std::size_t dataRowCount = PROCESSES_VISIBLE_ROWS;
                    if (g_processVisibleCount > dataRowCount)
                    {
                        std::size_t maxOffset = g_processVisibleCount - dataRowCount;
                        g_processScrollOffset += PROCESSES_VISIBLE_ROWS;
                        if (g_processScrollOffset > maxOffset)
                        {
                            g_processScrollOffset = maxOffset;
                        }
                    }
                }
                else if (key == SDLK_HOME)
                {
                    g_processScrollOffset = 0;
                    if (!g_visiblePids.empty())
                    {
                        g_selectedRowIndex = 0;
                        g_selectedPid = g_visiblePids.front();
                    }
                }
                else if (key == SDLK_END)
                {
                    std::size_t dataRowCount = PROCESSES_VISIBLE_ROWS;
                    if (g_processVisibleCount > dataRowCount)
                    {
                        g_processScrollOffset = g_processVisibleCount - dataRowCount;
                    }
                    else
                    {
                        g_processScrollOffset = 0;
                    }
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
                else if (key == SDLK_SPACE && g_treeMode && g_selectedPid >= 0)
                {
                    // Space collapses/expands the subtree rooted at the
                    // selected pid. State lives in g_collapsedPids and is
                    // consulted by the tree-mode flattener; the next poll
                    // hides (or restores) the descendants.
                    auto it = g_collapsedPids.find(g_selectedPid);
                    if (it == g_collapsedPids.end())
                    {
                        g_collapsedPids.insert(g_selectedPid);
                        flashStatus("collapsed pid " + std::to_string(g_selectedPid));
                    }
                    else
                    {
                        g_collapsedPids.erase(it);
                        flashStatus("expanded pid " + std::to_string(g_selectedPid));
                    }
                }
                break;
            }
            }
        }

        EventManager::getInstance().processEvents(ivec2(0, 0), ivec2(RESX - 1, RESY - 1));

        Uint64 now = SDL_GetTicks();
        if (now - lastPoll >= g_pollIntervalMs)
        {
            pollMetrics();
            lastPoll = now;
        }

        screen.clear();
        // Selected-row background. Painted BEFORE the layout render so the
        // row's text (drawn by the Row widget during LayoutManager render)
        // sits on top of the highlight box rather than under it. Renders
        // only on the Processes tab and only when a selection exists.
        if (processesTabActive() && g_selectedPid >= 0
            && g_selectedRowIndex >= 0
            && g_selectedRowIndex < PROCESSES_VISIBLE_ROWS)
        {
            int rowY = PROCESSES_FIRST_ROW_Y + g_selectedRowIndex * PROCESS_ROW_HEIGHT;
            const vec3 selectionBg(0.18f, 0.18f, 0.18f);
            screen.drawBox(ivec2(PROCESSES_ROW_X0, rowY),
                           ivec2(PROCESSES_ROW_X1, rowY + PROCESS_ROW_HEIGHT),
                           selectionBg);
        }
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
                renderAggregateDiskIo(screen);
                renderNetIo(screen);
                renderMemBreakdownStrip(screen);
                renderPerCoreStrip(screen);
                if (g_sparklinesEnabled)
                {
                    if (g_cpuSparkline != nullptr) g_cpuSparkline->draw(screen);
                    if (g_ramSparkline != nullptr) g_ramSparkline->draw(screen);
                    if (g_gpuSparkline != nullptr) g_gpuSparkline->draw(screen);
                    // tx (orange) renders behind rx (green) so incoming
                    // traffic stays the visually dominant line.
                    if (g_netTxSparkline != nullptr) g_netTxSparkline->draw(screen);
                    if (g_netRxSparkline != nullptr) g_netRxSparkline->draw(screen);
                    renderSparklineLabels(screen);
                }
            }
        }
        // Tree-mode collapse glyph overlay. The Row widget renders every
        // cell in a single textColor (white), so we repaint the leading
        // indent + '+ ' / '- ' fragment in a green/grey accent on top of
        // the row text it just drew. Same string at the same anchor, so
        // the cached glyph surface lines up exactly and the rest of the
        // name cell stays white. Skipped in flat mode and when nothing
        // visible.
        if (g_treeMode && processesTabActive())
        {
            const vec3 expandedColor(0.40f, 0.85f, 0.40f);
            const vec3 collapsedColor = Theme::textDim();
            // Name cell x = PROCESSES_ROW_X0 + weight[0] * rowWidth + 4
            // (same layout the Row widget uses to position cell text).
            int nameCellX = PROCESSES_ROW_X0
                          + static_cast<int>(0.10f
                              * static_cast<float>(PROCESSES_ROW_X1 - PROCESSES_ROW_X0))
                          + 4;
            std::size_t glyphRowCount = g_rowGlyphPrefix.size();
            if (glyphRowCount > static_cast<std::size_t>(PROCESSES_VISIBLE_ROWS))
            {
                glyphRowCount = static_cast<std::size_t>(PROCESSES_VISIBLE_ROWS);
            }
            for (std::size_t i = 0; i < glyphRowCount; ++i)
            {
                if (g_rowGlyphPrefix[i].empty())
                {
                    continue;
                }
                int rowY = PROCESSES_FIRST_ROW_Y
                         + static_cast<int>(i) * PROCESS_ROW_HEIGHT;
                vec3 color = g_rowGlyphCollapsed[i] ? collapsedColor : expandedColor;
                Font::drawText(screen, g_rowGlyphPrefix[i],
                               ivec2(nameCellX, rowY), color);
            }
        }
        renderFooterLiveStats(screen);

        // Aggregate summary strip sits above the Processes table column
        // headers (y=72) so a glance at the top of the tab shows the
        // running totals without summing the visible rows. Painted after
        // the layout tree so it is not overdrawn by the table chrome.
        if (processesTabActive())
        {
            renderProcessesSummary(screen);
        }

        // Process-kill overlays: selected-row highlight + signal menu + the
        // brief status flash after a send. Painted over the layout tree so
        // they sit on top of the Processes table without modifying the Row
        // widget itself.
        if (processesTabActive() && (g_filterInputActive || !g_filterText.empty()))
        {
            // Filter input strip at the top of the Processes tab, above
            // the header row. Two-tone: accent label, white query text,
            // a blinking cursor when input is active.
            const vec3 labelColor = Theme::accentGreen();
            const vec3 textColor(1.0f, 1.0f, 1.0f);
            const vec3 hintColor = Theme::textDim();
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
            const vec3 panelBorder = Theme::accentGreen();
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

        // Help overlay paints LAST so it sits on top of every tab and every
        // other overlay (signal menu, filter strip, selected-row highlight,
        // status flash). It is a read-only chrome surface so painting over
        // a transient state does not break anything.
        if (g_helpOverlayVisible)
        {
            renderHelpOverlay(screen);
        }

        screen.blitTo(SDL_GetWindowSurface(window));
        SDL_UpdateWindowSurface(window);
    }

    destroySparklines();
    SoundPlayer::getInstance().shutdown();
    Font::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();

    // Clean exit only: persist the four runtime knobs so the next run
    // remembers them. Failure is non-fatal; we just lose this snapshot.
    Settings::save(g_pollIntervalMs,
                   g_sparklinesEnabled,
                   static_cast<int>(g_sortColumn),
                   g_sortAscending,
                   g_collapsedPids);

    return 0;
}
