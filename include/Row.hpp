#ifndef __ROW_HPP__
#define __ROW_HPP__

#include <string>
#include <vector>
#include "GUIElement.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

class Row : public GUIElement
{
private:
    ivec2 minPos;
    ivec2 maxPos;
    std::vector<std::string> cells;
    std::vector<float> columnWeights;
    vec3 textColor;

public:
    Row(ivec2 minPos, ivec2 maxPos,
        std::vector<std::string> cells,
        std::vector<float> columnWeights,
        vec3 textColor);

    void setCells(std::vector<std::string> cells);
    const std::vector<std::string>& getCells() const;
    const std::vector<float>& getColumnWeights() const;
    ivec2 getMinPos() const;
    ivec2 getMaxPos() const;

    void draw(Screen &screen) override;
};

#endif
