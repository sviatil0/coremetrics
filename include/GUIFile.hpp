#ifndef __GUIFILE_HPP__
#define __GUIFILE_HPP__
#include "stdlib.h"
#include <fstream>
#include "GUIElements.hpp"

class GUIFile
{
private:
    std::vector<Point> points;
    std::vector<Line> lines;
    std::vector<Box> boxes;
    std::vector<std::string> tokens;
    std::vector<std::string> splitString(const std::string &str, char delim);

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