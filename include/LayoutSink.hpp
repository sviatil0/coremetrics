#ifndef __LAYOUTSINK_HPP__
#define __LAYOUTSINK_HPP__

#include <string>
#include <vector>

// LayoutSink: paint-rect introspection for the headless UI layout
// validator. When the sink is active, every Font::drawText() and
// Screen::drawBox() call records the rect it just painted. The
// --debug-layout CLI path enables the sink before rendering one tab
// headlessly, dumps the captured rects as JSON, and a Python audit
// script downstream checks for overlaps / out-of-bounds / contrast.
//
// Off by default. Zero cost when disabled (the paint primitives
// branch on an atomic flag before doing any work). Process-global,
// not thread-local: the render path is single-threaded.
namespace LayoutSink
{
    struct Rect
    {
        std::string kind;
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        std::string text;
        std::string fontSize;
        int r = 0;
        int g = 0;
        int b = 0;
    };

    void enable();
    void disable();
    bool isActive();
    void clear();

    void recordText(int x, int y, int w, int h,
                    const std::string &text,
                    const std::string &fontSize,
                    int r, int g, int b);
    void recordBox(int x, int y, int w, int h, int r, int g, int b);

    const std::vector<Rect> &rects();

    // Writes the captured rects as a JSON document to path. tab is
    // recorded in the payload so the audit script knows which tab the
    // shot came from. canvasW / canvasH let the audit detect
    // out-of-bounds rects without hard-coding the canvas size.
    bool writeJson(const std::string &path,
                   const std::string &tab,
                   int canvasW,
                   int canvasH);
}

#endif
