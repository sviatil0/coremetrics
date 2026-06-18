#include "HeadlessRunner.hpp"

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "AboutTab.hpp"
#include "AssetPath.hpp"
#include "Bar.hpp"
#include "DiskUsageRow.hpp"
#include "EmptyStates.hpp"
#include "EventManager.hpp"
#include "Exporter.hpp"
#include "FooterLiveStats.hpp"
#include "GUIFile.hpp"
#include "Label.hpp"
#include "Layout.hpp"
#include "LayoutManager.hpp"
#include "LayoutUtils.hpp"
#include "MemBreakdownStrip.hpp"
#include "MetricReadouts.hpp"
#include "MetricsPoller.hpp"
#include "NetIoFooter.hpp"
#include "PerCoreStrip.hpp"
#include "ProcessesSummary.hpp"
#include "SelfStats.hpp"
#include "Row.hpp"
#include "ShowEvent.hpp"
#include "Sparkline.hpp"
#include "SparklineLabels.hpp"
#include "SystemMetrics.hpp"
#include "TabIndicator.hpp"
#include "Theme.hpp"
#include "TopProcessesPrinter.hpp"
#include "Tree.hpp"
#include "UptimeAndLoad.hpp"
#include "font.hpp"
#include "screen.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 16 of the
// modernization roadmap. The previous in-place headless dispatch
// blocks lived back-to-back at the top of main() and each handled a
// CLI flag then returned without ever entering the SDL event loop.
// Together they were the single largest remaining extractable region
// in coremetrics.cpp; this TU keeps the same data ownership and
// consults what it does not own via extern.
//
// Symbols that moved here from coremetrics.cpp so the tests binary
// (which links every src/*.cpp object but not the coremetrics.cpp
// TU) resolves them at link time without a duplicate-definition
// clash. coremetrics.cpp now declares each of them extern at the
// top of the file.
//   g_sparklinesEnabled (CLI --sparklines toggle),
//   g_pollIntervalMs    (CLI --poll-ms override + Settings load/save),
//   g_pollLabel         (footer 'poll Xms' label pointer),
//   buildScene(),
//   cacheElementPointers(),
//   buildSparklines(),
//   destroySparklines().
// The four scene-build helpers moved together because the screenshot
// runner here and the live UI loop in coremetrics.cpp are their two
// call sites; coremetrics.cpp now reaches them through the same
// forward declarations the live UI loop already used. The
// SPARKLINE_CAPACITY and NET_SPARKLINE_MAX_KBPS constants moved with
// buildSparklines() since they were its sole consumers.
//
// Globals coremetrics.cpp / sibling TUs own that this TU reads
// through extern. The poller's snapshot symbols (g_cpuPct,
// g_memBreakdown, etc.) live in MetricsPoller.cpp; the filter /
// tree-mode symbols live in KeyboardEvents.cpp; the mute / sort
// symbols + the shutdown atomic live in MouseEvents.cpp; the
// sparkline + bar pointers live in MetricsPoller.cpp.

// Atomic the shutdown signal handler flips. Defined in
// src/MouseEvents.cpp (slice 14) so the EXIT button click can set
// it from that TU without a duplicate-symbol clash. The --top
// --watch loop polls it so Ctrl-C breaks the print cadence cleanly.
extern std::atomic<bool> g_shutdownRequested;

// Sparkline-enabled toggle. Lives in this TU as of slice 16 so the
// CLI parser in coremetrics.cpp writes it through extern on the way
// in, the screenshot path here reads it on the way through, and the
// tests binary (which links every src/*.cpp object but not the
// coremetrics.cpp TU) resolves the symbol at link time without a
// missing-extern. Initial value matches the v0.2 default.
bool g_sparklinesEnabled = false;

// Footer 'poll Xms' label pointer. Lives in this TU as of slice 16
// because cacheElementPointers() (also in this TU now) is its sole
// writer. coremetrics.cpp consults it from nowhere; the live UI
// path picks the runtime cadence up from g_pollIntervalMs directly.
Label *g_pollLabel = nullptr;

// Poll interval. Lives here as of slice 16 so the tests binary
// resolves the symbol via HeadlessRunner.o without depending on
// coremetrics.o; the CLI parser in coremetrics.cpp still owns the
// writers (Settings::load() seed + --poll-ms argv override +
// Settings::save() persist). cacheElementPointers() seeds the footer
// label text from it; runTopMode() takes its own copy by value via
// the watch-mode pollIntervalMs parameter so the watch cadence stays
// argv-aligned.
std::uint64_t g_pollIntervalMs = 500;

// Live snapshot globals owned by MetricsPoller.cpp (slice 15). The
// screenshot runner reads them after priming a fresh poll so the
// System / Processes / About tabs each paint a real metric strip
// rather than the zero-initialized state.
extern float g_cpuPct;
extern float g_ramPct;
extern float g_gpuPct;
extern std::vector<float> g_perCoreCpu;
extern MemBreakdown g_memBreakdown;
extern unsigned long long g_uptimeSeconds;
extern std::vector<float> g_loadAverages;
extern DiskUsage g_diskUsage;
extern NetIo g_netIo;
extern unsigned long long g_aggregateDiskReadKbPerSec;
extern unsigned long long g_aggregateDiskWriteKbPerSec;
extern std::size_t g_lastProcCount;
extern float g_sumCpuPct;
extern float g_sumMemPct;

// Sparkline owners. Defined in src/MetricsPoller.cpp (slice 15).
// Constructed by buildSparklines() (also in this TU); the screenshot
// path draws them once the scene is primed.
extern std::unique_ptr<Sparkline> g_cpuSparkline;
extern std::unique_ptr<Sparkline> g_ramSparkline;
extern std::unique_ptr<Sparkline> g_gpuSparkline;
extern std::unique_ptr<Sparkline> g_netRxSparkline;
extern std::unique_ptr<Sparkline> g_netTxSparkline;

// Bar widget pointers. Defined in src/MetricsPoller.cpp (slice 15).
// cacheElementPointers() in this TU is the sole writer; the live UI
// loop reads them every frame to recolor against the threshold.
extern Bar *g_cpuBar;
extern Bar *g_ramBar;
extern Bar *g_gpuBar;

// Live-UI state owned by KeyboardEvents.cpp (slice 13). The
// screenshot runner reads the filter text for the Processes-tab
// overlay strip; the rest sit behind the same processes-tab gate
// the live render path uses. cacheElementPointers() walks the row
// table to wire up the header row and the per-row references; the
// poller and the keyboard handler are the other writers.
extern std::string g_filterText;
extern std::size_t g_processScrollOffset;
extern std::size_t g_processVisibleCount;
extern std::vector<Row *> g_processRows;

// Mute label + header row pointers. Defined in src/MouseEvents.cpp
// (slice 14). cacheElementPointers() in this TU is the sole writer;
// the mouse handler reads them on every SOUND button + header click.
extern Label *g_muteLabel;
extern Row *g_headerRow;

// SOUND label recenter helper + sort-glyph repaint helper. Both
// defined in src/MouseEvents.cpp (slice 14). cacheElementPointers()
// in this TU calls them once the scene tree is loaded so the SOUND
// button reads pixel-centered and the header row carries the right
// sort glyph from the first frame.
void recenterMuteLabel();
void updateHeaderSortGlyph();

// Process-row height in pixels. Mirrors the live-UI constant in
// coremetrics.cpp so buildScene() can size the header column rects
// the same way the live UI does.
constexpr int kProcessRowHeight = 20;
// NET sparkline y-axis ceiling. Mirrors the live-UI constant in
// coremetrics.cpp so the rx/tx polylines clamp to the same range.
constexpr float kNetSparklineMaxKbps = 2048.0f;

// SOUND / EXIT button rects. Defined in src/MouseEvents.cpp (slice
// 14); buildScene() in this TU writes the four pairs once the scene
// is loaded so the mouse handler hit-tests against the same rects
// the SOUND label paints inside.
extern ivec2 g_muteBtnMin;
extern ivec2 g_muteBtnMax;
extern ivec2 g_exitBtnMin;
extern ivec2 g_exitBtnMax;
extern ivec2 g_headerColMin[5];
extern ivec2 g_headerColMax[5];

namespace
{
    // Mirror the live-UI canvas constants so the screenshot frame
    // matches base.xml exactly. coremetrics.cpp owns the live UI
    // copies; this TU keeps its own so the screenshot path stays
    // self-contained and a future tweak to the live UI window size
    // does not silently desync the screenshot dimensions.
    constexpr int kCanvasWidth = 960;
    constexpr int kCanvasHeight = 540;
    // Sparkline rolling-window capacity. Matches the live-UI value
    // so the headless prime loop fills exactly one window before the
    // shot lands.
    constexpr std::size_t kSparklineCapacity = 64;
    // Visible Processes-tab row count. Mirrors PROCESSES_VISIBLE_ROWS
    // in coremetrics.cpp so the Processes-tab summary strip clamps
    // its scroll math to the same row count the live UI does.
    constexpr int kProcessesVisibleRows = 15;

    // Shutdown handler the --top --watch loop wires up so Ctrl-C and
    // SIGTERM both break the print cadence cleanly. The atomic flag
    // is the same one the live UI loop and the EXIT button toggle.
    void handleShutdownSignal(int)
    {
        g_shutdownRequested.store(true);
    }

    // File-suffix helper used by the screenshot writer to pick BMP
    // vs PNG. Lifted from a previous in-place lambda inside main()
    // so the slice keeps the project rule of "no lambdas in app
    // code outside ThreadPool::submit" intact.
    bool endsWithSuffix(const std::string &s, const std::string &suffix)
    {
        return s.size() >= suffix.size()
               && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
}

// Scene-build helpers. Moved here from coremetrics.cpp as part of
// slice 16 so the screenshot runner and the live UI path both reach
// the same definitions and bin/tests resolves the symbols via
// HeadlessRunner.o without depending on coremetrics.o. coremetrics.cpp
// forward-declares each one; the live UI loop is the other caller.

void buildScene()
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
        g_headerColMax[c] = ivec2(colX + colW, rowY + kProcessRowHeight);
        colX += colW;
    }
}

void cacheElementPointers()
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

void buildSparklines()
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
        ivec2(24, 246), ivec2(864, 286), accent, 0.0f, 100.0f, kSparklineCapacity);
    g_cpuSparkline->setThresholdMode(true);
    g_ramSparkline = std::make_unique<Sparkline>(
        ivec2(24, 312), ivec2(864, 352), accent, 0.0f, 100.0f, kSparklineCapacity);
    g_ramSparkline->setThresholdMode(true);
    g_gpuSparkline = std::make_unique<Sparkline>(
        ivec2(24, 378), ivec2(864, 418), accent, 0.0f, 100.0f, kSparklineCapacity);
    g_gpuSparkline->setThresholdMode(true);
    // tx (orange) is constructed first so it renders behind the rx
    // (green) line in the draw pass; incoming traffic reads as more
    // important. NET strip ends at y=474 leaving 8px clear of the
    // y=482..516 EXIT button + footer chrome.
    const vec3 netTx(0.95f, 0.65f, 0.30f);
    const vec3 netRx(0.5f, 0.85f, 0.5f);
    g_netTxSparkline = std::make_unique<Sparkline>(
        ivec2(24, 444), ivec2(864, 474), netTx, 0.0f, kNetSparklineMaxKbps, kSparklineCapacity);
    g_netRxSparkline = std::make_unique<Sparkline>(
        ivec2(24, 444), ivec2(864, 474), netRx, 0.0f, kNetSparklineMaxKbps, kSparklineCapacity);
}

void destroySparklines()
{
    g_cpuSparkline.reset();
    g_ramSparkline.reset();
    g_gpuSparkline.reset();
    g_netRxSparkline.reset();
    g_netTxSparkline.reset();
}

namespace HeadlessRunner
{
    int runTopMode(int topCount,
                   TopSortKey topSortKey,
                   bool colorize,
                   bool watch,
                   unsigned long long pollIntervalMs)
    {
        if (topCount <= 0)
        {
            return -1;
        }
        // Install the signal handler before the loop so Ctrl-C exits
        // cleanly out of --watch instead of leaving a half-cleared
        // terminal. The same flag the GUI main loop watches.
        std::signal(SIGINT, handleShutdownSignal);
        std::signal(SIGTERM, handleShutdownSignal);
        if (!watch)
        {
            TopProcessesPrinter::print(topCount, topSortKey, colorize);
            return 0;
        }
        // --watch: clear the terminal with the ANSI 2J + cursor home
        // sequence then re-print. Cheap, no curses, works in any
        // terminal emulator. Uses pollIntervalMs (default 500 ms,
        // overridable via --poll-ms) so the cadence matches the GUI.
        while (!g_shutdownRequested.load())
        {
            std::printf("\033[2J\033[H");
            TopProcessesPrinter::print(topCount, topSortKey, colorize);
            SDL_Delay(static_cast<Uint32>(pollIntervalMs));
        }
        return 0;
    }

    int runExportMode(const std::string &csvPath, const std::string &jsonPath)
    {
        if (csvPath.empty() && jsonPath.empty())
        {
            return -1;
        }
        int exportStatus = 0;
        if (!csvPath.empty())
        {
            if (!Exporter::writeCsv(csvPath))
            {
                std::cerr << "Failed to write CSV export to "
                          << csvPath << '\n';
                exportStatus = 1;
            }
            else
            {
                std::cout << "Wrote CSV export to " << csvPath << '\n';
            }
        }
        if (!jsonPath.empty())
        {
            if (!Exporter::writeJson(jsonPath))
            {
                std::cerr << "Failed to write JSON export to "
                          << jsonPath << '\n';
                exportStatus = 1;
            }
            else
            {
                std::cout << "Wrote JSON export to " << jsonPath << '\n';
            }
        }
        return exportStatus;
    }

    int runScreenshotMode(const std::string &screenshotPath,
                          const std::string &screenshotTab)
    {
        if (screenshotPath.empty())
        {
            return -1;
        }
        // Headless --screenshot does not play sound, so initializing
        // the SDL audio subsystem is wasted work and on macOS the
        // AudioQueue background thread races the screenshot teardown,
        // producing a sporadic EXC_BAD_ACCESS in pthread_mutex_lock
        // during static destructors after the file has already been
        // written. Init only VIDEO here; the live UI path in
        // coremetrics.cpp still inits both subsystems for its own
        // SDL_Init call.
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            std::cerr << "Failed to init SDL: " << SDL_GetError() << '\n';
            return -1;
        }

        std::string tab = screenshotTab.empty() ? std::string("system")
                                                : screenshotTab;

        Screen shot(kCanvasWidth, kCanvasHeight);
        buildScene();
        cacheElementPointers();
        if (g_sparklinesEnabled)
        {
            buildSparklines();
            // Prime the sparklines so a single headless --screenshot
            // frame has enough history to draw a visible polyline.
            // 64 samples at 50ms each (~3.2s) fills the rolling
            // window exactly and also gives the per-process CPU%
            // delta a wide enough sampling window that idle
            // processes still register measurable ticks.
            for (std::size_t i = 0; i < kSparklineCapacity; ++i)
            {
                MetricsPoller::poll();
                SDL_Delay(50);
            }
        }
        else
        {
            // Per-process CPU% is a delta between two samples. 1.1s
            // was too tight: many processes accumulate sub-tick
            // activity in that window and round to 0.0% in the shot.
            // 3s captures everything that is doing real work without
            // making the screenshot path feel sluggish.
            MetricsPoller::poll();
            SDL_Delay(3000);
        }
        MetricsPoller::poll();

        if (tab == "processes")
        {
            EventManager::getInstance().pushEvent(
                std::make_unique<ShowEvent>("system", false));
            EventManager::getInstance().pushEvent(
                std::make_unique<ShowEvent>("processes", true));
            EventManager::getInstance().processEvents(
                ivec2(0, 0),
                ivec2(kCanvasWidth - 1, kCanvasHeight - 1));
        }
        else if (tab == "about")
        {
            EventManager::getInstance().pushEvent(
                std::make_unique<ShowEvent>("system", false));
            EventManager::getInstance().pushEvent(
                std::make_unique<ShowEvent>("about", true));
            EventManager::getInstance().processEvents(
                ivec2(0, 0),
                ivec2(kCanvasWidth - 1, kCanvasHeight - 1));
        }

        shot.clear();
        LayoutManager::getInstance().render(
            shot,
            ivec2(0, 0),
            ivec2(kCanvasWidth - 1, kCanvasHeight - 1));
        // Active-tab indicator: keeps the headless screenshot output
        // in sync with what the live loop paints (Pillar A5).
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
        // Self-monitoring badge: live RSS + CPU% on every tab so each
        // headless screenshot doubles as marketing for the lightweight
        // footprint (Pillar E).
        SelfStats::renderFooterBadge(shot);
        if (tab == "system")
        {
            // Dominant Title-size CPU/RAM/GPU readouts (Pillar A2).
            // Paints before the per-tab helpers so the smaller dim
            // "%" sign sits above the card background without
            // getting overdrawn.
            MetricReadouts::render(shot, g_cpuPct, g_ramPct, g_gpuPct);
            UptimeAndLoad::render(shot, g_uptimeSeconds, g_loadAverages);
            DiskUsageRow::render(shot, g_diskUsage);
            NetIoFooter::render(shot,
                                g_aggregateDiskReadKbPerSec,
                                g_aggregateDiskWriteKbPerSec,
                                g_netIo);
            MemBreakdownStrip::render(shot, g_memBreakdown);
            PerCoreStrip::render(shot, g_perCoreCpu, g_sparklinesEnabled);
            if (g_sparklinesEnabled)
            {
                if (g_cpuSparkline != nullptr) g_cpuSparkline->draw(shot);
                if (g_ramSparkline != nullptr) g_ramSparkline->draw(shot);
                if (g_gpuSparkline != nullptr) g_gpuSparkline->draw(shot);
                // tx (orange) first so the rx (green) line reads on
                // top; incoming traffic is the headline number for
                // most users.
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
                                     kProcessesVisibleRows);
            // Mirror the live loop's empty-state hint so the
            // screenshot matches what a user sees during the boot
            // moment (Pillar A6).
            if (g_lastProcCount == 0)
            {
                EmptyStates::renderProcessesEmpty(shot);
            }
        }
        else if (tab == "about")
        {
            AboutTab::render(shot, g_uptimeSeconds, g_perCoreCpu.size());
        }
        SDL_Surface *out = SDL_CreateSurface(
            kCanvasWidth, kCanvasHeight, SDL_PIXELFORMAT_RGBA32);
        if (out == nullptr)
        {
            std::cerr << "Failed to create screenshot surface: "
                      << SDL_GetError() << '\n';
            SDL_Quit();
            return -3;
        }
        shot.blitTo(out);

        // Pick the writer by extension so the same flag produces
        // what the README expects (PNG hero images) without forcing
        // callers to run an external converter step. Falls back to
        // BMP for any other suffix.
        bool saved = false;
        if (endsWithSuffix(screenshotPath, ".png")
            || endsWithSuffix(screenshotPath, ".PNG"))
        {
            saved = IMG_SavePNG(out, screenshotPath.c_str());
        }
        else
        {
            saved = SDL_SaveBMP(out, screenshotPath.c_str());
        }
        if (!saved)
        {
            std::cerr << "Failed to save screenshot: "
                      << SDL_GetError() << '\n';
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
}
