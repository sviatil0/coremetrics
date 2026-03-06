#include "vec2.hpp"
#include "vec3.hpp"
#include "screen.hpp"
#include "math.h"

//class Triangle : public GUIElement
class Triangle
{
private:
    vec2 v1, v2, v3;
    vec3 color;
    Screen &screen;

public:
    Triangle(vec2 v1, vec2 v2, vec2 v3, vec3 color, Screen &screen) : v1(v1), v2(v2), v3(v3), color(color), screen(screen)
    {
    }

    //void draw() override;
    int determinant2D(const vec2& a, const vec2& b, const vec2& c)
    void drawTriangle();
};

int crossEdge(const vec2& a, const vec2& b, const vec2& c)
{
    return (((b.y-a.y)*(c.x-a.x)) - ((b.x-a.x)*(c.y-a.y)));
}

void drawTriangle()
{
    //drawing help from: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage.html
    // get bounding coords to loop over (square)
    int minX = static_cast<int>(fmin(v1.x, fmin(v2.x, v3.x)));
    int minY = static_cast<int>(fmin(v1.y, fmin(v2.y, v3.y)));
    int maxX = static_cast<int>(fmax(v1.x, fmax(v2.x, v3.x)));
    int maxY = static_cast<int>(fmax(v1.y, fmax(v2.y, v3.y)));

    // within boundaries, check if each pixel is inside triangle or not
    for (int x = minX; x <= maxX; x++)
    {
        for (int y = minY; y <= maxY; y++)
        {
            vec2 p = vec2(x, y);
            int w0 = crossEdge(v2, v3, p);
            int w1 = crossEdge(v3, v1, p);
            int w2 = crossEdge(v1, v2, p);

            if (w0 >= 0 && w1 >= 0 && w2 >= 0)
            {
                drawPixel(p, color);
            }
        }
    }
}