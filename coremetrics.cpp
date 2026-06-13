#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
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
constexpr int PERCORE_X0 = 24;
constexpr int PERCORE_X1 = 936;
constexpr int PERCORE_GAP = 4;
static ivec2 g_headerColMin[4];
static ivec2 g_headerColMax[4];
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
    std::vector<float> weights = {0.15f, 0.5f, 0.175f, 0.175f};
    int headerRowWidth = 936 - 24;
    int colX = 24;
    for (int c = 0; c < 4; ++c)
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

static void renderPerCoreStrip(Screen &dest)
{
    if (g_perCoreCpu.empty())
    {
        return;
    }
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
    std::vector<ProcessInfo> procs = SystemMetrics::topProcesses(dataRowCount * 3);
    std::sort(procs.begin(), procs.end(), compareProcesses);
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
            std::vector<std::string> cells = {
                std::to_string(info.pid),
                info.name,
                formatPct(info.cpuPct),
                formatPct(info.memPct)
            };
            row->setCells(cells);
        }
        else
        {
            row->setCells({"", "", "", ""});
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
        if (tab != "processes")
        {
            renderPerCoreStrip(shot);
            if (g_sparklinesEnabled)
            {
                if (g_cpuSparkline != nullptr) g_cpuSparkline->draw(shot);
                if (g_ramSparkline != nullptr) g_ramSparkline->draw(shot);
                if (g_gpuSparkline != nullptr) g_gpuSparkline->draw(shot);
            }
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
                    for (int c = 0; c < 4; ++c)
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
                                std::vector<std::string> hdr = {"PID", "NAME", "CPU%", "MEM%"};
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
                    else
                    {
                        g_selectedPid = -1;
                        g_selectedRowIndex = -1;
                    }
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
                renderPerCoreStrip(screen);
                if (g_sparklinesEnabled)
                {
                    if (g_cpuSparkline != nullptr) g_cpuSparkline->draw(screen);
                    if (g_ramSparkline != nullptr) g_ramSparkline->draw(screen);
                    if (g_gpuSparkline != nullptr) g_gpuSparkline->draw(screen);
                }
            }
        }

        // Process-kill overlays: selected-row highlight + signal menu + the
        // brief status flash after a send. Painted over the layout tree so
        // they sit on top of the Processes table without modifying the Row
        // widget itself.
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
