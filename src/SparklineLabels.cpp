#include "SparklineLabels.hpp"
#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Pulled out of coremetrics.cpp as Phase 1.2 slice 2 of the GUI
// evolution spec. The four-line accent labels are pure paint operations
// with no state.

namespace SparklineLabels
{
    void render(Screen &dest)
    {
        const vec3 dim = Theme::textDim();
        // One label per sparkline, painted 12px above the chart's top
        // edge so the text sits in clear sky above the polyline fill
        // instead of getting overwritten on every peak. Chart rects are
        // CPU 246..286, RAM 312..352, GPU 378..418, NET 444..474.
        Font::drawText(dest, "CPU history", ivec2(24, 230), dim);
        Font::drawText(dest, "RAM history", ivec2(24, 296), dim);
        Font::drawText(dest, "GPU history", ivec2(24, 362), dim);
        Font::drawText(dest, "NET history", ivec2(24, 428), dim);
    }
}
