#ifndef __GAUGE_HPP__
#define __GAUGE_HPP__

#include "Cloneable.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Semicircular dial widget. The top half of a ring is rendered as a
// thresholded arc that fills from the left tip (value == minValue) to the
// right tip (value == maxValue), and a 3px needle is drawn from the
// circle's center out to the arc at the current value's angle.
//
// The arc is rasterized with the same pixel-scan strategy used by
// ProgressRing: walk every pixel of the bounding box, accept those whose
// distance from center lies inside the annulus
// [radius - arcThickness, radius], and filter by polar angle. No SDL
// circle primitive is used; the entire drawing pipeline stays in this
// project's own rasterizer (Screen::drawPixel for the arc, Screen::drawLine
// for the needle), in line with the from-scratch policy that already
// governs Bar, Sparkline, and Label.
//
// Angle convention. Screen-space y grows downward, so the visible top
// half of the circle is described by theta in [pi, 2*pi] in screen-space
// polar coordinates. setValue maps the clamped value linearly onto that
// range: minValue -> pi (left tip), maxValue -> 2*pi (right tip). The
// needle endpoint is (center.x + cos(theta)*radius, center.y +
// sin(theta)*radius); since sin(theta) is negative on (pi, 2*pi) the
// needle always points into the visible (upper) half of the screen.
class Gauge : public Cloneable<Gauge>
{
private:
    ivec2 center;
    int radius;
    int arcThickness;
    float value;
    float minValue;
    float maxValue;
    vec3 arcColor;
    vec3 needleColor;
    vec3 bgColor;

public:
    Gauge(ivec2 center, int radius, int arcThickness,
          vec3 arcColor, vec3 needleColor, vec3 bgColor,
          float minValue = 0.0f, float maxValue = 100.0f);

    void setValue(float v);
    float getValue() const;
    ivec2 getCenter() const;
    int getRadius() const;
    int getArcThickness() const;

    void draw(Screen &screen) override;
};

#endif
