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
#include "Bar.hpp"
#include "Row.hpp"
#include "label.hpp"
#include "button.hpp"
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

static Bar *g_cpuBar = nullptr;
static Bar *g_ramBar = nullptr;
static Label *g_cpuReadout = nullptr;
static Label *g_ramReadout = nullptr;
static std::vector<Row *> g_processRows;

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

    int btnWidth = (RESX / 2) - (TAB_BTN_PAD * 2);
    int btnY = TAB_BTN_PAD;
    int btnMaxY = TAB_BAR_HEIGHT - TAB_BTN_PAD;

    tabbar->getData().addElement(std::make_unique<Button>(
        ivec2(TAB_BTN_PAD, btnY),
        ivec2(TAB_BTN_PAD + btnWidth, btnMaxY),
        vec3(0.2f, 0.4f, 0.7f),
        "",
        "system",
        "processes"));
    tabbar->getData().addElement(std::make_unique<Label>("System",
        ivec2(TAB_BTN_PAD + 12, btnY + 6),
        vec3(1.0f, 1.0f, 1.0f)));

    tabbar->getData().addElement(std::make_unique<Button>(
        ivec2(TAB_BTN_PAD + btnWidth + TAB_BTN_PAD * 2, btnY),
        ivec2(RESX - TAB_BTN_PAD, btnMaxY),
        vec3(0.2f, 0.4f, 0.7f),
        "",
        "processes",
        "system"));
    tabbar->getData().addElement(std::make_unique<Label>("Processes",
        ivec2(TAB_BTN_PAD + btnWidth + TAB_BTN_PAD * 2 + 12, btnY + 6),
        vec3(1.0f, 1.0f, 1.0f)));

    float tabContentStartY = static_cast<float>(TAB_BAR_HEIGHT) / static_cast<float>(RESY);

    Tree<Layout> *system = manager.addChild(&root,
        Layout(vec2(0.0f, tabContentStartY), vec2(1.0f, 1.0f), true, "system"));

    int cpuY = TAB_BAR_HEIGHT + BAR_MARGIN;
    int ramY = cpuY + BAR_HEIGHT + BAR_MARGIN;

    int readoutWidth = 72;
    int barMaxX = RESX - BAR_MARGIN - readoutWidth;

    system->getData().addElement(std::make_unique<Label>("CPU",
        ivec2(BAR_MARGIN, cpuY + 4),
        vec3(1.0f, 1.0f, 1.0f)));
    system->getData().addElement(std::make_unique<Bar>(
        ivec2(BAR_MARGIN + BAR_LABEL_WIDTH, cpuY),
        ivec2(barMaxX, cpuY + BAR_HEIGHT),
        vec3(0.2f, 0.8f, 0.2f),
        vec3(0.15f, 0.15f, 0.15f),
        0.0f, 100.0f, "cpu"));
    system->getData().addElement(std::make_unique<Label>("0.0%",
        ivec2(barMaxX + 8, cpuY + 4),
        vec3(1.0f, 1.0f, 1.0f)));

    system->getData().addElement(std::make_unique<Label>("RAM",
        ivec2(BAR_MARGIN, ramY + 4),
        vec3(1.0f, 1.0f, 1.0f)));
    system->getData().addElement(std::make_unique<Bar>(
        ivec2(BAR_MARGIN + BAR_LABEL_WIDTH, ramY),
        ivec2(barMaxX, ramY + BAR_HEIGHT),
        vec3(0.8f, 0.6f, 0.2f),
        vec3(0.15f, 0.15f, 0.15f),
        0.0f, 100.0f, "ram"));
    system->getData().addElement(std::make_unique<Label>("0.0%",
        ivec2(barMaxX + 8, ramY + 4),
        vec3(1.0f, 1.0f, 1.0f)));

    Tree<Layout> *processes = manager.addChild(&root,
        Layout(vec2(0.0f, tabContentStartY), vec2(1.0f, 1.0f), false, "processes"));

    std::vector<float> weights = {0.15f, 0.5f, 0.175f, 0.175f};
    std::vector<std::string> header = {"PID", "NAME", "CPU%", "MEM%"};
    int rowY = TAB_BAR_HEIGHT + PROCESS_TOP_PADDING;
    processes->getData().addElement(std::make_unique<Row>(
        ivec2(BAR_MARGIN, rowY),
        ivec2(RESX - BAR_MARGIN, rowY + PROCESS_ROW_HEIGHT),
        header, weights, vec3(1.0f, 1.0f, 0.4f)));

    for (std::size_t i = 0; i < PROCESS_ROW_COUNT; ++i)
    {
        int y = rowY + PROCESS_ROW_HEIGHT + PROCESS_TOP_PADDING + static_cast<int>(i) * PROCESS_ROW_HEIGHT;
        std::vector<std::string> emptyCells = {"", "", "", ""};
        processes->getData().addElement(std::make_unique<Row>(
            ivec2(BAR_MARGIN, y),
            ivec2(RESX - BAR_MARGIN, y + PROCESS_ROW_HEIGHT),
            emptyCells, weights, vec3(0.9f, 0.9f, 0.9f)));
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

    SDL_Window *window = SDL_CreateWindow("CoreMetrics", RESX, RESY, 0);
    if (window == nullptr)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return -2;
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
                EventManager::getInstance().pushEvent(std::make_unique<ClickEvent>(
                    static_cast<int>(clickX), static_cast<int>(clickY)));
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

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
