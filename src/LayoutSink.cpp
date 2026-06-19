#include "LayoutSink.hpp"

#include <atomic>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace LayoutSink
{
    namespace
    {
        std::atomic<bool> g_active{false};
        std::vector<Rect> g_rects;

        // JSON string escape: doubles backslashes and quotes, drops
        // control bytes. Label text is the only field that can carry
        // user-influenced characters (process names land in the
        // Processes tab). Kept conservative on purpose: an audit
        // script consuming the JSON does not need fancy unicode
        // handling, just a parseable payload.
        std::string escapeJson(const std::string &s)
        {
            std::string out;
            out.reserve(s.size() + 2);
            for (char c : s)
            {
                if (c == '\\')
                {
                    out += "\\\\";
                }
                else if (c == '"')
                {
                    out += "\\\"";
                }
                else if (c == '\n')
                {
                    out += "\\n";
                }
                else if (c == '\t')
                {
                    out += "\\t";
                }
                else if (static_cast<unsigned char>(c) < 0x20)
                {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x",
                                  static_cast<unsigned int>(c));
                    out += buf;
                }
                else
                {
                    out.push_back(c);
                }
            }
            return out;
        }
    }

    void enable()
    {
        g_active.store(true);
        g_rects.clear();
    }

    void disable()
    {
        g_active.store(false);
    }

    bool isActive()
    {
        return g_active.load();
    }

    void clear()
    {
        g_rects.clear();
    }

    void recordText(int x, int y, int w, int h,
                    const std::string &text,
                    const std::string &fontSize,
                    int r, int g, int b)
    {
        if (!g_active.load())
        {
            return;
        }
        Rect rect;
        rect.kind = "text";
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
        rect.text = text;
        rect.fontSize = fontSize;
        rect.r = r;
        rect.g = g;
        rect.b = b;
        g_rects.push_back(rect);
    }

    void recordBox(int x, int y, int w, int h, int r, int g, int b)
    {
        if (!g_active.load())
        {
            return;
        }
        Rect rect;
        rect.kind = "box";
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
        rect.r = r;
        rect.g = g;
        rect.b = b;
        g_rects.push_back(rect);
    }

    const std::vector<Rect> &rects()
    {
        return g_rects;
    }

    bool writeJson(const std::string &path,
                   const std::string &tab,
                   int canvasW,
                   int canvasH)
    {
        std::ofstream out(path);
        if (!out.is_open())
        {
            return false;
        }
        out << "{\n";
        out << "  \"tab\": \"" << escapeJson(tab) << "\",\n";
        out << "  \"canvas\": { \"w\": " << canvasW
            << ", \"h\": " << canvasH << " },\n";
        out << "  \"rects\": [";
        bool first = true;
        for (const auto &r : g_rects)
        {
            if (!first)
            {
                out << ",";
            }
            first = false;
            out << "\n    {";
            out << "\"kind\": \"" << r.kind << "\"";
            out << ", \"x\": " << r.x;
            out << ", \"y\": " << r.y;
            out << ", \"w\": " << r.w;
            out << ", \"h\": " << r.h;
            out << ", \"rgb\": [" << r.r << ", " << r.g << ", " << r.b << "]";
            if (r.kind == "text")
            {
                out << ", \"text\": \"" << escapeJson(r.text) << "\"";
                out << ", \"size\": \"" << r.fontSize << "\"";
            }
            out << "}";
        }
        out << "\n  ]\n}\n";
        return out.good();
    }
}
