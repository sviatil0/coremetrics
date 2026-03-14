#include "GUIFile.hpp"

GUIFile::GUIFile() {} // do we need to do anything with the constructor?

void GUIFile::setPoint(Point point)
{
}
void GUIFile::setLine(Line line)
{
}
void GUIFile::setBox(Box box)
{
}

std::vector<Point> getPoints()
{
}
std::vector<Line> getLines()
{
}
std::vector<Box> getBoxes()
{
}

void readFile(std::string fileName)
{
    if (!(std::filesystem::exists(fileName)))
    {
        return;
    }
}
void writeFile(std::string fileName)
{
    if (!(std::filesystem::exists(fileName)))
    {
        return;
    }
}
