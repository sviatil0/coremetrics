#include <algorithm>
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
#include "SoundEvent.hpp"
#include "SoundPlayer.hpp"
#include "font.hpp"
#include "GUIElements.hpp"
#include "GUIFile.hpp"
#include "SystemMetrics.hpp"
#include "LayoutUtils.hpp"
#include "ProcessUtils.hpp"

constexpr int RESX = 960;
constexpr int RESY = 540;
constexpr int TAB_BAR_HEIGHT = 48;
constexpr int TAB_BTN_PAD = 8;
constexpr int BAR_HEIGHT = 24;
constexpr int BAR_MARGIN = 24;
constexpr int BAR_LABEL_WIDTH = 60;
constexpr int PROCESS_ROW_HEIGHT = 20;
constexpr int PROCESS_TOP_PADDING = 16;
constexpr std::size_t PROCESS_ROW_COUNT = 15;
constexpr Uint64 POLL_INTERVAL_MS = 500;

constexpr float ALARM_THRESHOLD = 80.0f;
constexpr const char *ALARM_SOUND_PATH = "assets/click.wav";

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

static SortColumn g_sortColumn = SORT_MEM;
static bool g_sortAscending = false;
static Row *g_headerRow = nullptr;
static ivec2 g_headerColMin[4];
static ivec2 g_headerColMax[4];
static ivec2 g_muteBtnMin;
static ivec2 g_muteBtnMax;
static ivec2 g_exitBtnMin;
static ivec2 g_exitBtnMax;

static bool compareProcesses(const ProcessInfo &a, const ProcessInfo &b)
{
    switch (g_sortColumn)
    {
    case SORT_PID:
        return g_sortAscending ? (a.pid < b.pid) : (a.pid > b.pid);
    case SORT_NAME:
        return g_sortAscending ? (a.name < b.name) : (a.name > b.name);
    case SORT_CPU:
        return g_sortAscending ? (a.cpuPct < b.cpuPct) : (a.cpuPct > b.cpuPct);
    case SORT_MEM:
        return g_sortAscending ? (a.memPct < b.memPct) : (a.memPct > b.memPct);
    }
    return false;
}

static void buildScene()
{
    LayoutManager &manager = LayoutManager::getInstance();
    Tree<Layout> &root = manager.getRoot();
    GUIFile g;
    g.readFile("base.xml", manager);

    int muteBtnWidth = 140;
    int tabsTotalWidth = RESX - (TAB_BTN_PAD * 4) - muteBtnWidth;
    int btnWidth = tabsTotalWidth / 2;
    int btnY = TAB_BTN_PAD;
    int btnMaxY = TAB_BAR_HEIGHT - TAB_BTN_PAD;
    g_muteBtnMin = ivec2(TAB_BTN_PAD * 3 + btnWidth * 2, btnY);
    g_muteBtnMax = ivec2(g_muteBtnMin.x + muteBtnWidth, btnMaxY);

    int exitBtnSize = 64;
    int exitBtnPad = 16;
    g_exitBtnMin = ivec2(RESX - exitBtnSize - exitBtnPad, RESY - exitBtnSize - exitBtnPad);
    g_exitBtnMax = ivec2(g_exitBtnMin.x + exitBtnSize, g_exitBtnMin.y + exitBtnSize);

    float tabContentStartY = static_cast<float>(TAB_BAR_HEIGHT) / static_cast<float>(RESY);

    Tree<Layout> *system = manager.addChild(&root,
        Layout(vec2(0.0f, tabContentStartY), vec2(1.0f, 1.0f), true, "system"));

    int cpuY = TAB_BAR_HEIGHT + BAR_MARGIN;
    int ramY = cpuY + BAR_HEIGHT + BAR_MARGIN;
    int gpuY = ramY + BAR_HEIGHT + BAR_MARGIN;

    int readoutWidth = 72;
    int barMaxX = RESX - BAR_MARGIN - readoutWidth;

    auto baseLabel = std::make_unique<Label>("0.0%",
        ivec2(barMaxX + 8, cpuY + 4),
        COLOR_TEXT_ACCENT);

    system->getData().addElement(std::make_unique<Label>("CPU",
        ivec2(BAR_MARGIN, cpuY + 4),
        COLOR_TEXT_PRIMARY));
    system->getData().addElement(std::make_unique<Bar>(
        ivec2(BAR_MARGIN + BAR_LABEL_WIDTH, cpuY),
        ivec2(barMaxX, cpuY + BAR_HEIGHT),
        COLOR_BAR_CPU_FILL,
        COLOR_BAR_BG,
        0.0f, 100.0f, "cpu"));
    system->getData().addElement(cloneUnique(*baseLabel));

    system->getData().addElement(std::make_unique<Label>("RAM",
        ivec2(BAR_MARGIN, ramY + 4),
        COLOR_TEXT_PRIMARY));
    system->getData().addElement(std::make_unique<Bar>(
        ivec2(BAR_MARGIN + BAR_LABEL_WIDTH, ramY),
        ivec2(barMaxX, ramY + BAR_HEIGHT),
        COLOR_BAR_RAM_FILL,
        COLOR_BAR_BG,
        0.0f, 100.0f, "ram"));
    baseLabel->setPos(ivec2(barMaxX + 8, ramY + 4));
    system->getData().addElement(cloneUnique(*baseLabel));


    system->getData().addElement(std::make_unique<Label>("GPU",
        ivec2(BAR_MARGIN, gpuY + 4),
        COLOR_TEXT_PRIMARY));
    system->getData().addElement(std::make_unique<Bar>(
        ivec2(BAR_MARGIN + BAR_LABEL_WIDTH, gpuY),
        ivec2(barMaxX, gpuY + BAR_HEIGHT),
        COLOR_BAR_CPU_FILL,
        COLOR_BAR_BG,
        0.0f, 100.0f, "gpu"));
    baseLabel->setPos(ivec2(barMaxX + 8, gpuY + 4));
    system->getData().addElement(cloneUnique(*baseLabel));


    Tree<Layout> *processes = manager.addChild(&root,
        Layout(vec2(0.0f, tabContentStartY), vec2(1.0f, 1.0f), false, "processes"));

    std::vector<float> weights = {0.15f, 0.5f, 0.175f, 0.175f};
    std::vector<std::string> header = {"PID", "NAME", "CPU%", "MEM%"};
    int rowY = TAB_BAR_HEIGHT + PROCESS_TOP_PADDING;

    int headerRowWidth = (RESX - BAR_MARGIN) - BAR_MARGIN;
    int colX = BAR_MARGIN;
    for (int c = 0; c < 4; ++c)
    {
        int colW = static_cast<int>(weights[c] * static_cast<float>(headerRowWidth));
        g_headerColMin[c] = ivec2(colX, rowY);
        g_headerColMax[c] = ivec2(colX + colW, rowY + PROCESS_ROW_HEIGHT);
        colX += colW;
    }

    processes->getData().addElement(std::make_unique<Row>(
        ivec2(BAR_MARGIN, rowY),
        ivec2(RESX - BAR_MARGIN, rowY + PROCESS_ROW_HEIGHT),
        header, weights, COLOR_ROW_HEADER));

    for (std::size_t i = 0; i < PROCESS_ROW_COUNT; ++i)
    {
        int y = rowY + PROCESS_ROW_HEIGHT + PROCESS_TOP_PADDING + static_cast<int>(i) * PROCESS_ROW_HEIGHT;
        std::vector<std::string> emptyCells = {"", "", "", ""};
        processes->getData().addElement(std::make_unique<Row>(
            ivec2(BAR_MARGIN, y),
            ivec2(RESX - BAR_MARGIN, y + PROCESS_ROW_HEIGHT),
            emptyCells, weights, COLOR_ROW_TEXT));
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
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cerr << "Failed to init SDL: " << SDL_GetError() << '\n';
        return -1;
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

    SDL_Surface *iconSurface = IMG_Load("assets/logo.png");
    if (iconSurface != nullptr)
    {
        SDL_SetWindowIcon(window, iconSurface);
        SDL_DestroySurface(iconSurface);
    }

    Screen screen(RESX, RESY);

    buildScene();
    cacheElementPointers();
    pollMetrics();

    Uint64 lastPoll = SDL_GetTicks();
    SDL_Event event;
    bool end = false;

    while (!end)
    {
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
                        EventManager::getInstance().pushEvent(std::make_unique<ClickEvent>(mx, my));
                    }
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
        screen.blitTo(SDL_GetWindowSurface(window));
        SDL_UpdateWindowSurface(window);
    }

    SoundPlayer::getInstance().shutdown();
    Font::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
