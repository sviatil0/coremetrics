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
        // One Caption-size label per sparkline, painted in the 18px
        // band above each chart so the text sits in clear sky above
        // the polyline fill instead of getting overwritten on every
        // peak. Caption instead of Body: the 27px Body surface at the
        // old y=230 collided with the per-core strip (y=218..236);
        // the 14pt Caption face fits the band. Chart rects are
        // CPU 260..300, RAM 318..358, GPU 376..416, NET 434..474
        // (buildSparklines() in HeadlessRunner.cpp; lockstep).
        Font::drawText(dest, "CPU history", ivec2(24, 242), dim, Font::Size::Caption);
        Font::drawText(dest, "RAM history", ivec2(24, 300), dim, Font::Size::Caption);
        Font::drawText(dest, "GPU history", ivec2(24, 358), dim, Font::Size::Caption);
        Font::drawText(dest, "NET history", ivec2(24, 416), dim, Font::Size::Caption);
    }
}
