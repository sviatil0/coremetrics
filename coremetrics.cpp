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
#include "HelpOverlay.hpp"
#include "SparklineLabels.hpp"
#include "MetricReadouts.hpp"
#include "UptimeAndLoad.hpp"
#include "NetIoFooter.hpp"
#include "DiskUsageRow.hpp"
#include "MemBreakdownStrip.hpp"
#include "PerCoreStrip.hpp"
#include "FooterLiveStats.hpp"
#include "ProcessesSummary.hpp"
#include "SignalMenuOverlay.hpp"
#include "TopProcessesPrinter.hpp"
#include "FilterStrip.hpp"
#include "StatusFlash.hpp"
#include "Thresholds.hpp"
#include "Theme.hpp"
#include "AssetPath.hpp"
#include "SignalUtils.hpp"
#include "KillLog.hpp"
#include "Exporter.hpp"
#include "Settings.hpp"
#include "TabIndicator.hpp"
#include "EmptyStates.hpp"
#include "AboutTab.hpp"
#include "KeyboardEvents.hpp"
#include "MouseEvents.hpp"
#include "MetricsPoller.hpp"

constexpr int RESX = 960;
constexpr int RESY = 540;
constexpr int PROCESS_ROW_HEIGHT = 20;
// Default poll interval is 500ms; overridable at startup via the
// --poll-ms <N> CLI flag (clamped to [100, 10000] so the UI cannot
// be told to repaint faster than the OS can serve samples or so slow
// the chrome looks frozen).
static Uint64 g_pollIntervalMs = 500;
// Bound constants live in src/ProcessUtils.cpp alongside the
// clampPollIntervalMs() helper that uses them.

// ALARM_THRESHOLD / ALARM_SOUND_PATH moved to src/MetricsPoller.cpp
// (Phase 1.2 slice 15) alongside the alarm rising-edge detector that
// owns the only call site for them.

// Bar widget pointers. Defined in src/MetricsPoller.cpp (Phase 1.2
// slice 15) so the tests binary (which links every src/*.cpp object
// but not the coremetrics.cpp TU) resolves the symbols the poller
// references. buildScene() / cacheElementPointers() in this TU is
// still the sole writer of the pointer values; the poller drives
// setValue / setFillColor each tick.
extern Bar *g_cpuBar;
extern Bar *g_ramBar;
extern Bar *g_gpuBar;
// Latest sampled percentages, kept at file scope so the render loop can
// hand them to MetricReadouts::render(...) outside of the poller. The
// XML labels that used to display these were removed when the readouts
// moved to programmatic Title-size paint (Pillar A2 wire-up). Defined
// in src/MetricsPoller.cpp (Phase 1.2 slice 15) so the writer owns the
// data; this TU consults them every frame to paint the System tab.
extern float g_cpuPct;
extern float g_ramPct;
extern float g_gpuPct;
// Defined in src/MouseEvents.cpp (Phase 1.2 slice 14). The mouse
// handler swaps its text between "SOUND ON" and "SOUND OFF" when
// the user toggles the alarm; buildScene() in this TU assigns the
// pointer once the scene tree is loaded.
extern Label *g_muteLabel;
static Label *g_pollLabel = nullptr;
// Defined in src/KeyboardEvents.cpp (Phase 1.2 slice 13) so the live-UI
// state-machine globals all live next to the handler that owns them.
extern std::vector<Row *> g_processRows;

// Defined in src/MouseEvents.cpp (Phase 1.2 slice 14). The mouse
// handler is the sole user-driven writer (SOUND button toggle); the
// alarm-flash sites in this TU still read it every tick.
extern bool g_alarmEnabled;
// Alarm rising-edge detector state. Defined in src/MetricsPoller.cpp
// (Phase 1.2 slice 15) where poll() is the only reader + writer.

// Set by SIGINT/SIGTERM and checked by the main loop. The main loop also
// exits on its own once an optional --duration is exceeded; this flag covers
// Ctrl-C from a terminal and `kill` from a parent process or CI. Defined
// in src/MouseEvents.cpp so the EXIT button click can set it from that TU
// without a duplicate-symbol clash.
extern std::atomic<bool> g_shutdownRequested;

static void handleShutdownSignal(int)
{
    g_shutdownRequested.store(true);
}

// Defined in src/MouseEvents.cpp (Phase 1.2 slice 14). The mouse
// handler writes them when the user clicks a Processes-table header;
// the sort comparator, render path, and Settings save/load all read.
extern SortColumn g_sortColumn;
extern bool g_sortAscending;
// Same TU. buildScene() in this TU assigns the pointer once the
// header row is built; updateHeaderSortGlyph() consults it on every
// header click to repaint the active column's direction glyph.
extern Row *g_headerRow;

// Sparklines are moonshot UI: a rolling time-series chart per metric drawn
// over the System tab's lower half. Behind a flag (--sparklines) so the
// default install matches the audit baseline; with the flag the System tab
// gains 3 live polylines fed by RingBuffer<float> samples.
static bool g_sparklinesEnabled = false;
constexpr std::size_t SPARKLINE_CAPACITY = 64;
// Sparkline owners. Defined in src/MetricsPoller.cpp (Phase 1.2
// slice 15) so the tests link, mirroring the bar pointers above.
// buildSparklines() / destroySparklines() in this TU is still the
// sole writer of the unique_ptr itself; the poller pushes a new
// sample into each polyline every tick.
extern std::unique_ptr<Sparkline> g_cpuSparkline;
extern std::unique_ptr<Sparkline> g_ramSparkline;
extern std::unique_ptr<Sparkline> g_gpuSparkline;
// Network throughput history: two polylines overlaid in the same rect
// directly below the GPU strip. rx in accent green, tx in orange so a
// reviewer can read incoming vs outgoing traffic at a glance without a
// legend. y-range fixed at 0..2048 KB/s so the chart stays a stable
// comparison surface across ticks (samples are clamped, not stretched).
// Net sparklines, defined in src/MetricsPoller.cpp (Phase 1.2 slice
// 15) for the same tests-link reason as the CPU/RAM/GPU triple above.
extern std::unique_ptr<Sparkline> g_netRxSparkline;
extern std::unique_ptr<Sparkline> g_netTxSparkline;
constexpr float NET_SPARKLINE_MAX_KBPS = 2048.0f;

// Per-logical-CPU utilization for the small strip below the aggregate bars.
// Refreshed every poll. Empty on Windows (backend not implemented yet) and
// on the very first poll (no prior tick sample to diff against). Defined
// in src/MetricsPoller.cpp (Phase 1.2 slice 15) where poll() refreshes it
// each tick; this TU still consults it from the render pass + AboutTab.
extern std::vector<float> g_perCoreCpu;

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

// Defined in src/KeyboardEvents.cpp so the live-UI state lives next to
// the handler that mutates it. coremetrics.cpp still reads + writes
// these from the mouse handler, polling loop, and CLI / scene setup.
extern int g_selectedPid;
extern int g_selectedRowIndex;
extern std::vector<int> g_visiblePids;
// First-row index in the fetched process list. PgDown/PgUp shift the
// visible window by PROCESSES_VISIBLE_ROWS; up/down arrow at the edge
// of the visible window scrolls by one. Stays clamped to
// [0, max(0, procCount - dataRowCount)] so the bottom row of data
// fills the last visible slot even on overscroll.
extern std::size_t g_processScrollOffset;
// Snapshot of the most recent procs.size() after filter+truncate, set
// by pollMetrics for the key handler so PgDown / arrow keys can clamp
// against the actual table length without recomputing.
extern std::size_t g_processVisibleCount;
// Per-visible-row glyph overlay state for tree mode. When non-empty,
// the prefix string contains the indent spaces + '+ ' or '- ' that
// should be re-painted in color over the white text the Row already
// drew. Same length as g_visiblePids; empty string means no glyph
// (leaf row or flat mode). Parallel bool flags whether the row is
// collapsed (true => grey '+') or expanded (false => green '-').
// Defined in src/MetricsPoller.cpp (Phase 1.2 slice 15). The poller
// writes the parallel arrays during the tree-mode flatten pass; this
// TU's render path reads them to repaint the leading '+ ' / '- '
// glyph in green/grey on top of the Row widget's white text.
extern std::vector<std::string> g_rowGlyphPrefix;
extern std::vector<bool> g_rowGlyphCollapsed;

// Defined in src/KeyboardEvents.cpp. closeSignalMenu() below resets
// them; the menu render path reads them every frame.
extern bool g_signalMenuVisible;
extern int g_signalMenuPickedIndex; // -1 = picking, 0..5 = awaiting confirm

// Defined in src/KeyboardEvents.cpp alongside flashStatus(). The
// renderer below reads them every tick to decide whether to paint
// the "sent TERM -> pid N" footer line.
extern std::string g_statusFlash;
extern Uint64 g_statusFlashExpiryMs;

// Process search/filter. '/' on the Processes tab enters input mode.
// g_filterText is the case-insensitive substring matched against process
// names; an empty string disables filtering. g_filterInputActive tracks
// whether keystrokes append to the filter (input mode) or trigger the
// usual row navigation (filter applied but no longer being edited).
// Defined in src/KeyboardEvents.cpp. coremetrics.cpp still consults
// the text + flag when filtering the row table each tick.
extern std::string g_filterText;
extern bool g_filterInputActive;

// Tree mode: when on, the Processes table groups parent/child pairs
// depth-first and indents the name cell with tree connectors. Toggled
// by 't' on the Processes tab. When off the table sorts by the active
// column the way it has since the SAFE wave.
// Defined in src/KeyboardEvents.cpp. Tree-mode tick reads the flag
// each poll to decide whether to flatten parent/child rows.
extern bool g_treeMode;
// Per-pid collapse state for tree mode. Pids in this set hide their
// descendants in the flattened row list; pressing 'space' on a row
// toggles membership. Initial state: empty (everything expanded).
// Kept across tree-mode toggles so the user's layout sticks.
// Defined in src/KeyboardEvents.cpp.
extern std::unordered_set<int> g_collapsedPids;

// Help overlay: a translucent-feel dark panel listing every hotkey,
// toggled by '?' and dismissed with '?' or Esc. Painted last in the
// render pass so it sits on top of every tab and every other overlay.
// Defined in src/KeyboardEvents.cpp; consulted at render time here.
extern bool g_helpOverlayVisible;

// Defined in src/KeyboardEvents.cpp; declared here so callers in
// this TU (mouse handler, scene builder, alarm flash sites) can
// post status flashes without duplicating the body.
void flashStatus(const std::string &text);

// Defined in src/KeyboardEvents.cpp; declared here so the mouse
// handler and the polling loop can guard tab-specific behavior.
bool processesTabActive();

static bool aboutTabActive()
{
    Tree<Layout> *node = EventManager::findLayoutByName(
        LayoutManager::getInstance().getRoot(), "about");
    return node != nullptr && node->getData().isActive();
}

// Mute and tab-button geometries are cached at scene build time. The
// three tab buttons in base.xml each declare a target + hide pair, but
// the XML schema only supports one hide per button. With three tabs a
// click on "About" needs to hide both "system" and "processes"; rather
// than extend Button to accept a list of hides we intercept tab clicks
// in the main loop (the same pattern the EXIT and SOUND buttons
// already use) and toggle the layout active flags directly. The cached
// rects stay in lockstep with base.xml by reviewer convention.
//
// Defined in src/MouseEvents.cpp (Phase 1.2 slice 14) so the handler
// that hit-tests them owns the data. buildScene() in this TU still
// initializes the EXIT, SOUND, and header column rects each launch.
extern ivec2 g_systemTabBtnMin;
extern ivec2 g_systemTabBtnMax;
extern ivec2 g_processesTabBtnMin;
extern ivec2 g_processesTabBtnMax;
extern ivec2 g_aboutTabBtnMin;
extern ivec2 g_aboutTabBtnMax;

// SOUND toggle button rect. Initialized in buildScene(). Defined in
// src/MouseEvents.cpp (Phase 1.2 slice 14) alongside the other
// hit-test rects; recenterMuteLabel() reads it via the same extern.
extern ivec2 g_muteBtnMin;
extern ivec2 g_muteBtnMax;

// Defined in src/MouseEvents.cpp (Phase 1.2 slice 14) alongside the
// SOUND button click path that toggles the label. Forward-declared
// here so buildScene() can re-center the label after assigning
// g_muteLabel for the first time.
void recenterMuteLabel();

// Defined in src/KeyboardEvents.cpp; declared here so the mouse
// handler can dismiss the menu when the click misses the panel.
void closeSignalMenu();
// PERCORE_* geometry moved to src/PerCoreStrip.cpp alongside the
// renderer that owns the strip.

// Memory breakdown segmented bar. Sits just below the aggregate RAM bar
// (y=136..160 in base.xml). Shows active / wired / cached / free segments
// at the same horizontal extent so they read as a continuation. htop
// colors: active = red, wired = orange, cached = blue, free = dark.
// All six snapshot globals below are defined in src/MetricsPoller.cpp
// (Phase 1.2 slice 15) where poll() refreshes them each tick; this TU
// consults them from the render pass to paint the System tab strips.
extern MemBreakdown g_memBreakdown;
extern unsigned long long g_uptimeSeconds;
extern std::vector<float> g_loadAverages;
// Root volume capacity sampled every poll. UI shows
// "DISK <used>/<total> GB (NN%)" with the same yellow/red threshold
// recolor as the RAM bar so a full root volume reads as visually
// urgent. Zero totalKb means the backend failed; the strip is hidden.
extern DiskUsage g_diskUsage;
// Aggregate disk I/O rate summed across the top-N processes the
// backend returned. Shown on the System tab as 'I/O R X.X MB/s
// W Y.Y MB/s' so reviewers see system-wide disk pressure without
// scanning the per-process table.
extern unsigned long long g_aggregateDiskReadKbPerSec;
extern unsigned long long g_aggregateDiskWriteKbPerSec;
// Aggregate network rx/tx sampled every poll. Loopback excluded.
extern NetIo g_netIo;
// MEMSEG_* geometry moved to src/MemBreakdownStrip.cpp alongside the
// renderer that owns the strip.

// Defined in src/MouseEvents.cpp (Phase 1.2 slice 14). buildScene()
// in this TU initializes them once the scene tree is loaded; the
// mouse handler reads them every click.
extern ivec2 g_headerColMin[5];
extern ivec2 g_headerColMax[5];
// g_muteBtnMin/Max declared earlier alongside recenterMuteLabel().
extern ivec2 g_exitBtnMin;
extern ivec2 g_exitBtnMax;

// compareProcesses() moved to src/MetricsPoller.cpp (Phase 1.2 slice 15)
// alongside the std::sort call site that is its only consumer.

// Defined in src/MouseEvents.cpp (Phase 1.2 slice 14) alongside the
// header-row click path that drives the sort toggle. Forward-declared
// here so buildScene() can paint the initial glyph after the header
// row pointer is wired up.
void updateHeaderSortGlyph();

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

    g_gpuBar = findBarByMetric(root, "gpu");

    Tree<Layout> *tabbarNode = EventManager::findLayoutByName(root, "tabbar");
    if (tabbarNode != nullptr)
    {
        // Tabbar labels in document order: 0 System, 1 Processes, 2 About,
        // 3 SOUND ON/OFF. Pre-v3 code used index 2 which silently pointed
        // at the "About" tab label and would mutate the tab title every
        // time the user toggled sound; nobody noticed because the alarm
        // strings happened to overwrite "About" with comparable-width
        // characters. Index 3 is the actual SOUND label.
        g_muteLabel = nthLabelInLayout(*tabbarNode, 3);
        // Recenter the SOUND label from its XML default once the TTF face is
        // initialized so the static "SOUND ON" string ends up pixel-centered
        // inside the button rect; subsequent toggles re-run the same logic.
        recenterMuteLabel();
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

// Uptime + Load row paint moved to src/UptimeAndLoad.cpp as Phase 1.2
// slice 3 of the GUI evolution spec. Call site passes g_uptimeSeconds
// and g_loadAverages explicitly via UptimeAndLoad::render(...).

// Old formatRate(kbPerSec) helper deleted; superseded by the
// compact-rate formatter inside src/NetIoFooter.cpp (slice 4 extract).

// NetIo footer paint moved to src/NetIoFooter.cpp as Phase 1.2 slice 4
// of the GUI evolution spec. Call site passes the three globals
// explicitly via NetIoFooter::render(...).

// Disk usage row paint moved to src/DiskUsageRow.cpp as Phase 1.2
// slice 5 of the GUI evolution spec. Call site passes g_diskUsage
// explicitly via DiskUsageRow::render(...).

// Memory breakdown strip paint moved to src/MemBreakdownStrip.cpp as
// Phase 1.2 slice 6 of the GUI evolution spec. Call site passes
// g_memBreakdown explicitly via MemBreakdownStrip::render(...).

// Per-core CPU strip paint moved to src/PerCoreStrip.cpp as Phase 1.2
// slice 7 of the GUI evolution spec. Call site passes g_perCoreCpu +
// g_sparklinesEnabled explicitly via PerCoreStrip::render(...).

// Live footer string replacing the v0.1.x static "alarm threshold 80%"
// label. Same color and y-baseline as the other footer chrome
// (coremetrics / v0.2.0 / poll 500ms) so it reads as a continuation.
// Latest process count is set by MetricsPoller::poll(). Defined in
// src/MetricsPoller.cpp (Phase 1.2 slice 15).
extern std::size_t g_lastProcCount;
// Latest summed CPU% / MEM% across the topProcesses sample. Maintained
// alongside g_lastProcCount by MetricsPoller::poll() and read by the
// Processes tab summary strip so a reviewer can eyeball aggregate
// pressure without summing the visible table.
extern float g_sumCpuPct;
extern float g_sumMemPct;

// Footer "N procs" live count moved to src/FooterLiveStats.cpp as
// Phase 1.2 slice 8 of the GUI evolution spec. Reaches the same pixels
// via FooterLiveStats::render(dest, g_lastProcCount).

// Processes-tab aggregate strip moved to src/ProcessesSummary.cpp as
// Phase 1.2 slice 9 of the GUI evolution spec. Reaches the same pixels
// via ProcessesSummary::render(dest, g_lastProcCount, ...).

// Help overlay body moved to src/HelpOverlay.cpp as the first slice of
// Phase 1.2 from the GUI evolution spec (PR #163). Reaches the same
// pixels via HelpOverlay::render(dest).

// Tiny accent labels at the left edge of each sparkline strip. Without
// them three unlabeled polylines at the bottom of the System tab read as
// abstract decoration; with them a reviewer instantly sees they're CPU
// / RAM / GPU history. y-baseline picked to sit just above each strip.
// Sparkline label paint moved to src/SparklineLabels.cpp as Phase 1.2
// slice 2 of the GUI evolution spec. Reaches the same pixels via
// SparklineLabels::render(dest).

// pollMetrics() moved to src/MetricsPoller.cpp as Phase 1.2 slice 15
// of the modernization roadmap. The previous 362-line in-place body
// is now MetricsPoller::poll(); every call site below dispatches
// through that namespace function. The poller owns the metric
// snapshot globals it produces (g_cpuPct, g_memBreakdown, etc.) and
// consults the scene widget pointers + live-UI state via extern.

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
    TopSortKey topSortKey = TopSortKey::Mem;
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

    // --help / -h print the same flag reference the manpage carries.
    // --version / -V print a one-line semver string. Both exit 0
    // before any other state is touched.
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--version" || arg == "-V")
        {
            // Mirrors base.xml's footer label; bumped by the release
            // flow that touches the XML.
            std::printf("coremetrics 0.3.0\n");
            return 0;
        }
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
                "  --screenshot PATH [system|processes|about]\n"
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
                "  -V, --version             print version and exit\n"
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
                topSortKey = TopSortKey::Mem;
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
            TopProcessesPrinter::print(topCount, topSortKey, topColorize);
            return 0;
        }
        // --watch: clear the terminal with the ANSI 2J + cursor home
        // sequence then re-print. Cheap, no curses, works in any
        // terminal emulator. Uses g_pollIntervalMs (defaults 500 ms,
        // overridable via --poll-ms) so the cadence matches the GUI.
        while (!g_shutdownRequested.load())
        {
            std::printf("\033[2J\033[H");
            TopProcessesPrinter::print(topCount, topSortKey, topColorize);
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
                MetricsPoller::poll();
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
            MetricsPoller::poll();
            SDL_Delay(3000);
        }
        MetricsPoller::poll();

        if (tab == "processes")
        {
            EventManager::getInstance().pushEvent(std::make_unique<ShowEvent>("system", false));
            EventManager::getInstance().pushEvent(std::make_unique<ShowEvent>("processes", true));
            EventManager::getInstance().processEvents(ivec2(0, 0), ivec2(RESX - 1, RESY - 1));
        }
        else if (tab == "about")
        {
            EventManager::getInstance().pushEvent(std::make_unique<ShowEvent>("system", false));
            EventManager::getInstance().pushEvent(std::make_unique<ShowEvent>("about", true));
            EventManager::getInstance().processEvents(ivec2(0, 0), ivec2(RESX - 1, RESY - 1));
        }

        shot.clear();
        LayoutManager::getInstance().render(shot, ivec2(0, 0), ivec2(RESX - 1, RESY - 1));
        // Active-tab indicator: keeps the headless screenshot output in
        // sync with what the live loop paints (Pillar A5).
        {
            int activeTabIndex = 0;
            if (tab == "about")
            {
                activeTabIndex = 2;
            }
            else if (tab == "processes")
            {
                activeTabIndex = 1;
            }
            TabIndicator::render(shot, activeTabIndex);
        }
        FooterLiveStats::render(shot, g_lastProcCount);
        if (tab == "system")
        {
            // Dominant Title-size CPU/RAM/GPU readouts (Pillar A2). Paints
            // before the per-tab helpers so the smaller dim "%" sign sits
            // above the card background without getting overdrawn.
            MetricReadouts::render(shot, g_cpuPct, g_ramPct, g_gpuPct);
            UptimeAndLoad::render(shot, g_uptimeSeconds, g_loadAverages);
            DiskUsageRow::render(shot, g_diskUsage);
            NetIoFooter::render(shot, g_aggregateDiskReadKbPerSec, g_aggregateDiskWriteKbPerSec, g_netIo);
            MemBreakdownStrip::render(shot, g_memBreakdown);
            PerCoreStrip::render(shot, g_perCoreCpu, g_sparklinesEnabled);
            if (g_sparklinesEnabled)
            {
                if (g_cpuSparkline != nullptr) g_cpuSparkline->draw(shot);
                if (g_ramSparkline != nullptr) g_ramSparkline->draw(shot);
                if (g_gpuSparkline != nullptr) g_gpuSparkline->draw(shot);
                // tx (orange) first so the rx (green) line reads on top;
                // incoming traffic is the headline number for most users.
                if (g_netTxSparkline != nullptr) g_netTxSparkline->draw(shot);
                if (g_netRxSparkline != nullptr) g_netRxSparkline->draw(shot);
                SparklineLabels::render(shot);
            }
        }
        else if (tab == "processes" && !g_filterText.empty())
        {
            const vec3 labelColor = Theme::accentGreen();
            const vec3 textColor(1.0f, 1.0f, 1.0f);
            Font::drawText(shot, "filter: ", ivec2(24, 44), labelColor);
            Font::drawText(shot, g_filterText, ivec2(120, 44), textColor);
        }
        if (tab == "processes")
        {
            ProcessesSummary::render(shot,
                                     g_lastProcCount,
                                     g_sumCpuPct,
                                     g_sumMemPct,
                                     g_processScrollOffset,
                                     g_processVisibleCount,
                                     PROCESSES_VISIBLE_ROWS);
            // Mirror the live loop's empty-state hint so the screenshot
            // matches what a user sees during the boot moment (Pillar A6).
            if (g_lastProcCount == 0)
            {
                EmptyStates::renderProcessesEmpty(shot);
            }
        }
        else if (tab == "about")
        {
            AboutTab::render(shot, g_uptimeSeconds, g_perCoreCpu.size());
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
    MetricsPoller::poll();

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
                MouseEvents::handleButtonDown(event.button);
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL:
            {
                MouseEvents::handleWheel(event.wheel);
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
                KeyboardEvents::handle(event.key);
                break;
            }
            }
        }

        EventManager::getInstance().processEvents(ivec2(0, 0), ivec2(RESX - 1, RESY - 1));

        Uint64 now = SDL_GetTicks();
        if (now - lastPoll >= g_pollIntervalMs)
        {
            MetricsPoller::poll();
            lastPoll = now;
        }

        screen.clear(Theme::bgBase());
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

        // Active-tab indicator stripe. Painted right after the layout
        // tree so it sits on top of the tab-strip background but under
        // the per-tab overlays below. tabIndex derives from the live
        // layout state so the stripe always tracks the visible content
        // layout. Pillar A5 of the modernization roadmap.
        {
            int activeTabIndex = 0;
            if (aboutTabActive())
            {
                activeTabIndex = 2;
            }
            else if (processesTabActive())
            {
                activeTabIndex = 1;
            }
            TabIndicator::render(screen, activeTabIndex);
        }
        if (aboutTabActive())
        {
            AboutTab::render(screen, g_uptimeSeconds, g_perCoreCpu.size());
        }
        {
            // Per-core strip and sparklines only paint while the System tab
            // is the active layout. On Processes, the rows occupy the same
            // pixel range, so painting either would overdraw the table.
            Tree<Layout> *systemNode = EventManager::findLayoutByName(
                LayoutManager::getInstance().getRoot(), "system");
            if (systemNode != nullptr && systemNode->getData().isActive())
            {
                // Dominant Title-size CPU/RAM/GPU readouts (Pillar A2).
                // Mirrors the screenshot path; same System-tab gate.
                MetricReadouts::render(screen, g_cpuPct, g_ramPct, g_gpuPct);
                UptimeAndLoad::render(screen, g_uptimeSeconds, g_loadAverages);
                DiskUsageRow::render(screen, g_diskUsage);
                NetIoFooter::render(screen, g_aggregateDiskReadKbPerSec, g_aggregateDiskWriteKbPerSec, g_netIo);
                MemBreakdownStrip::render(screen, g_memBreakdown);
                PerCoreStrip::render(screen, g_perCoreCpu, g_sparklinesEnabled);
                if (g_sparklinesEnabled)
                {
                    if (g_cpuSparkline != nullptr) g_cpuSparkline->draw(screen);
                    if (g_ramSparkline != nullptr) g_ramSparkline->draw(screen);
                    if (g_gpuSparkline != nullptr) g_gpuSparkline->draw(screen);
                    // tx (orange) renders behind rx (green) so incoming
                    // traffic stays the visually dominant line.
                    if (g_netTxSparkline != nullptr) g_netTxSparkline->draw(screen);
                    if (g_netRxSparkline != nullptr) g_netRxSparkline->draw(screen);
                    SparklineLabels::render(screen);
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
        FooterLiveStats::render(screen, g_lastProcCount);

        // Aggregate summary strip sits above the Processes table column
        // headers (y=72) so a glance at the top of the tab shows the
        // running totals without summing the visible rows. Painted after
        // the layout tree so it is not overdrawn by the table chrome.
        if (processesTabActive())
        {
            ProcessesSummary::render(screen,
                                     g_lastProcCount,
                                     g_sumCpuPct,
                                     g_sumMemPct,
                                     g_processScrollOffset,
                                     g_processVisibleCount,
                                     PROCESSES_VISIBLE_ROWS);
        }

        // Empty-state hint. When the Processes tab is active but the
        // backend has not yet returned a sample, the layout would
        // otherwise paint a blank 16-row table with no explanation.
        // Painted after the row chrome so it lands on top of the empty
        // rows rather than under them. The current gate covers the
        // boot moment only; a future revision should also fire when an
        // active filter excludes every visible row (g_lastProcCount is
        // set pre-filter, so a separate visible-row count is needed).
        // Pillar A6 of the modernization roadmap.
        if (processesTabActive() && g_lastProcCount == 0)
        {
            EmptyStates::renderProcessesEmpty(screen);
        }

        // Process-kill overlays: selected-row highlight + signal menu + the
        // brief status flash after a send. Painted over the layout tree so
        // they sit on top of the Processes table without modifying the Row
        // widget itself.
        if (processesTabActive() && (g_filterInputActive || !g_filterText.empty()))
        {
            FilterStrip::render(screen, g_filterInputActive, g_filterText, SDL_GetTicks());
        }

        if (processesTabActive() && g_selectedRowIndex >= 0
            && g_selectedRowIndex < PROCESSES_VISIBLE_ROWS)
        {
            int y0 = PROCESSES_FIRST_ROW_Y + g_selectedRowIndex * PROCESS_ROW_HEIGHT;
            int y1 = y0 + PROCESS_ROW_HEIGHT - 1;
            // Thin accent strip on the left edge of the row. Cheap to draw,
            // does not overdraw the text in the row. Brand-blue Theme::accent()
            // matches the active-tab indicator introduced in #210 so the strip
            // reads as "selected by user input" rather than the load-pressure
            // green that the old COLOR_ACCENT_GREEN was suggesting.
            screen.drawBox(ivec2(PROCESSES_ROW_X0 - 6, y0),
                           ivec2(PROCESSES_ROW_X0 - 2, y1),
                           Theme::accent());
        }

        if (g_signalMenuVisible)
        {
            SignalMenuOverlay::render(screen, g_selectedPid, g_signalMenuPickedIndex);
        }

        if (!g_statusFlash.empty() && SDL_GetTicks() < g_statusFlashExpiryMs)
        {
            // Paint the flash over the right end of the footer status strip,
            // before the EXIT button. The accent color is reused so the flash
            // reads as part of the same status row, not a foreign popup.
            StatusFlash::render(screen, g_statusFlash);
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
            HelpOverlay::render(screen);
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
