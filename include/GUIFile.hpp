#ifndef __GUIFILE_HPP__
#define __GUIFILE_HPP__
#include <stdlib.h>
#include <vector>
#include <map>
#include <stack>
#include <string>
#include <fstream>
#include "GUIElements.hpp"

class GUIFile
{
private:
    std::vector<Point> points;
    std::vector<Line> lines;
    std::vector<Box> boxes;
    std::vector<std::string> tokens;
    std::string getContent(const std::string &source, const std::string &tag, size_t &pos);
    Layout parseLayout(const std::string &block);
    vec2 parseVec2(const std::string &block, size_t &p);
    vec3 parseVec3(const std::string &block, size_t &p);

public:
    GUIFile() = default;
    //setter/staging methods
    void setPoint(Point);
    void setLine(Line);
    void setBox(Box);
    //getter methods
    std::vector<Point> getPoints();
    std::vector<Line> getLines();
    std::vector<Box> getBoxes();
    //file reading/parsing and writing
    void readFile(std::string fileName);
    void writeFile(std::string fileName);
};
#endif