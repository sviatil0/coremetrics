#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include "screen.hpp"
#include "LayoutManager.hpp"
#include "EventManager.hpp"
#include "ClickEvent.hpp"
#include "SoundEvent.hpp"
#include "SoundPlayer.hpp"
#include "font.hpp"
#include "Bar.hpp"
#include "Box.hpp"
#include "Row.hpp"
#include "label.hpp"
#include "button.hpp"
#include "image.hpp"
#include "SystemMetrics.hpp"

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
static Label *g_cpuReadout = nullptr;
static Label *g_ramReadout = nullptr;
static Label *g_muteLabel = nullptr;
static std::vector<Row *> g_processRows;

static bool g_alarmEnabled = true;
static bool g_cpuAlarmActive = false;
static bool g_ramAlarmActive = false;
static ivec2 g_muteBtnMin;
static ivec2 g_muteBtnMax;
static ivec2 g_exitBtnMin;
static ivec2 g_exitBtnMax;

static Bar *findBarByMetric(Tree<Layout> &node, const std::string &metric)
{
    for (const auto &element : node.getData().elements)
    {
        Bar *bar = dynamic_cast<Bar *>(element.get());
        if (bar != nullptr && bar->getMetricName() == metric)
        {
            return bar;
        }
    }
    for (auto &child : node.getChildren())
    {
        Bar *bar = findBarByMetric(*child, metric);
        if (bar != nullptr)
        {
            return bar;
        }
    }
    return nullptr;
}

static Tree<Layout> *findLayoutNode(Tree<Layout> &node, const std::string &name)
{
    if (node.getData().getName() == name)
    {
        return &node;
    }
    for (auto &child : node.getChildren())
    {
        Tree<Layout> *found = findLayoutNode(*child, name);
        if (found != nullptr)
        {
            return found;
        }
    }
    return nullptr;
}

static Label *nthLabelInLayout(Tree<Layout> &layoutNode, std::size_t index)
{
    std::size_t seen = 0;
    for (const auto &element : layoutNode.getData().elements)
    {
        Label *label = dynamic_cast<Label *>(element.get());
        if (label == nullptr)
        {
            continue;
        }
        if (seen == index)
        {
            return label;
        }
        ++seen;
    }
    return nullptr;
}

static void collectRows(Tree<Layout> &node, std::vector<Row *> &out)
{
    for (const auto &element : node.getData().elements)
    {
        Row *row = dynamic_cast<Row *>(element.get());
        if (row != nullptr)
        {
            out.push_back(row);
        }
    }
    for (auto &child : node.getChildren())
    {
        collectRows(*child, out);
    }
}

static std::string formatPct(float value)
{
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << value;
    return oss.str();
}

static void buildScene()
{
    LayoutManager &manager = LayoutManager::getInstance();
    Tree<Layout> &root = manager.getRoot();

    Tree<Layout> *tabbar = manager.addChild(&root,
        Layout(vec2(0.0f, 0.0f), vec2(1.0f, static_cast<float>(TAB_BAR_HEIGHT) / static_cast<float>(RESY)), true, "tabbar"));

    int iconSize = 32;
    int iconX = TAB_BTN_PAD;
    int iconY = (TAB_BAR_HEIGHT - iconSize) / 2;
    tabbar->getData().addElement(std::make_unique<Image>("assets/logo.png",
        ivec2(iconX, iconY)));

    int tabsStartX = TAB_BTN_PAD * 2 + iconSize;
    int muteBtnWidth = 140;
    int tabsTotalWidth = RESX - tabsStartX - TAB_BTN_PAD * 3 - muteBtnWidth;
    int btnWidth = tabsTotalWidth / 2;
    int btnY = TAB_BTN_PAD;
    int btnMaxY = TAB_BAR_HEIGHT - TAB_BTN_PAD;

    tabbar->getData().addElement(std::make_unique<Button>(
        ivec2(tabsStartX, btnY),
        ivec2(tabsStartX + btnWidth, btnMaxY),
        COLOR_TAB_ACTIVE,
        "",
        "system",
        "processes"));
    tabbar->getData().addElement(std::make_unique<Label>("System",
        ivec2(tabsStartX + 12, btnY + 6),
        COLOR_TEXT_PRIMARY));

    int procBtnX = tabsStartX + btnWidth + TAB_BTN_PAD;
    tabbar->getData().addElement(std::make_unique<Button>(
        ivec2(procBtnX, btnY),
        ivec2(procBtnX + btnWidth, btnMaxY),
        COLOR_TAB_ACTIVE,
        "",
        "processes",
        "system"));
    tabbar->getData().addElement(std::make_unique<Label>("Processes",
        ivec2(procBtnX + 12, btnY + 6),
        COLOR_TEXT_PRIMARY));

    g_muteBtnMin = ivec2(procBtnX + btnWidth + TAB_BTN_PAD, btnY);
    g_muteBtnMax = ivec2(g_muteBtnMin.x + muteBtnWidth, btnMaxY);

    tabbar->getData().addElement(std::make_unique<Button>(
        g_muteBtnMin,
        g_muteBtnMax,
        COLOR_MUTE_BTN,
        "", "", ""));
    tabbar->getData().addElement(std::make_unique<Label>("SOUND ON",
        ivec2(g_muteBtnMin.x + 6, btnY + 6),
        COLOR_TEXT_PRIMARY));

    int exitBtnSize = 64;
    int exitBtnPad = 16;
    g_exitBtnMin = ivec2(RESX - exitBtnSize - exitBtnPad, RESY - exitBtnSize - exitBtnPad);
    g_exitBtnMax = ivec2(g_exitBtnMin.x + exitBtnSize, g_exitBtnMin.y + exitBtnSize);
    root.getData().addElement(std::make_unique<Button>(
        g_exitBtnMin,
        g_exitBtnMax,
        COLOR_EXIT_BTN,
        "", "", ""));
    root.getData().addElement(std::make_unique<Label>("EXIT",
        ivec2(g_exitBtnMin.x + 16, g_exitBtnMin.y + 22),
        COLOR_TEXT_PRIMARY));

    int logoIconSize = 32;
    int logoIconX = TAB_BTN_PAD / 2;
    int logoIconY = (TAB_BAR_HEIGHT - logoIconSize) / 2;
    tabbar->getData().addElement(std::make_unique<Image>("assets/logo.png",
        ivec2(logoIconX, logoIconY)));

    float tabContentStartY = static_cast<float>(TAB_BAR_HEIGHT) / static_cast<float>(RESY);

    Tree<Layout> *system = manager.addChild(&root,
        Layout(vec2(0.0f, tabContentStartY), vec2(1.0f, 1.0f), true, "system"));

    int cpuY = TAB_BAR_HEIGHT + BAR_MARGIN;
    int ramY = cpuY + BAR_HEIGHT + BAR_MARGIN;

    int readoutWidth = 72;
    int barMaxX = RESX - BAR_MARGIN - readoutWidth;

    system->getData().addElement(std::make_unique<Label>("CPU",
        ivec2(BAR_MARGIN, cpuY + 4),
        COLOR_TEXT_PRIMARY));
    system->getData().addElement(std::make_unique<Bar>(
        ivec2(BAR_MARGIN + BAR_LABEL_WIDTH, cpuY),
        ivec2(barMaxX, cpuY + BAR_HEIGHT),
        COLOR_BAR_CPU_FILL,
        COLOR_BAR_BG,
        0.0f, 100.0f, "cpu"));
    system->getData().addElement(std::make_unique<Label>("0.0%",
        ivec2(barMaxX + 8, cpuY + 4),
        COLOR_TEXT_ACCENT));

    system->getData().addElement(std::make_unique<Label>("RAM",
        ivec2(BAR_MARGIN, ramY + 4),
        COLOR_TEXT_PRIMARY));
    system->getData().addElement(std::make_unique<Bar>(
        ivec2(BAR_MARGIN + BAR_LABEL_WIDTH, ramY),
        ivec2(barMaxX, ramY + BAR_HEIGHT),
        COLOR_BAR_RAM_FILL,
        COLOR_BAR_BG,
        0.0f, 100.0f, "ram"));
    system->getData().addElement(std::make_unique<Label>("0.0%",
        ivec2(barMaxX + 8, ramY + 4),
        COLOR_TEXT_ACCENT));

    Tree<Layout> *processes = manager.addChild(&root,
        Layout(vec2(0.0f, tabContentStartY), vec2(1.0f, 1.0f), false, "processes"));

    std::vector<float> weights = {0.15f, 0.5f, 0.175f, 0.175f};
    std::vector<std::string> header = {"PID", "NAME", "CPU%", "MEM%"};
    int rowY = TAB_BAR_HEIGHT + PROCESS_TOP_PADDING;
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

    Tree<Layout> *systemNode = findLayoutNode(root, "system");
    if (systemNode != nullptr)
    {
        g_cpuReadout = nthLabelInLayout(*systemNode, 1);
        g_ramReadout = nthLabelInLayout(*systemNode, 3);
    }

    Tree<Layout> *tabbarNode = findLayoutNode(root, "tabbar");
    if (tabbarNode != nullptr)
    {
        g_muteLabel = nthLabelInLayout(*tabbarNode, 2);
    }

    g_processRows.clear();
    collectRows(root, g_processRows);
}

static void pollMetrics()
{
    float cpuPct = SystemMetrics::readCpuPercent();
    float memPct = SystemMetrics::readMemPercent();

    if (g_cpuBar != nullptr)
    {
        g_cpuBar->setValue(cpuPct);
    }
    if (g_ramBar != nullptr)
    {
        g_ramBar->setValue(memPct);
    }
    if (g_cpuReadout != nullptr)
    {
        g_cpuReadout->setText(formatPct(cpuPct) + "%");
    }
    if (g_ramReadout != nullptr)
    {
        g_ramReadout->setText(formatPct(memPct) + "%");
    }

    bool cpuNowAlarm = cpuPct >= ALARM_THRESHOLD;
    bool ramNowAlarm = memPct >= ALARM_THRESHOLD;
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
    }
    g_cpuAlarmActive = cpuNowAlarm;
    g_ramAlarmActive = ramNowAlarm;

    if (g_processRows.size() <= 1)
    {
        return;
    }

    std::size_t dataRowCount = g_processRows.size() - 1;
    std::vector<ProcessInfo> procs = SystemMetrics::topProcesses(dataRowCount);

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
                    EventManager::getInstance().pushEvent(std::make_unique<ClickEvent>(mx, my));
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
