#include "GUIFile.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

GUIFile::GUIFile() {} //do we need to do anything with the constructor?

void GUIFile::setPoint(Point point)
{
    points.push_back(point);
}

void GUIFile::setLine(Line line)
{
    lines.push_back(line);
}

void GUIFile::setBox(Box box)
{
    boxes.push_back(box);
}

std::vector<Point> GUIFile::getPoints()
{
    return points;
}

std::vector<Line> GUIFile::getLines()
{
    return lines;
}

std::vector<Box> GUIFile::getBoxes()
{
    return boxes;
}

void readFile(std::string fileName)
{
    if (!(std::filesystem::exists(fileName)))
    {
        return;
    }

}

void GUIFile::writeFile(std::string fileName)
{
    std::ofstream outFile(fileName);

    if (!outFile.is_open())
    {
        return;
    }

    outFile << "<layout>\n";

    for (const auto& line : lines)
    {
        outFile << "  <line>\n";
        outFile << "    <vec2>\n";
        outFile << "      <x>" << line.start.x << "</x>\n";
        outFile << "      <y>" << line.start.y << "</y>\n";
        outFile << "    </vec2>\n";
        outFile << "    <vec2>\n";
        outFile << "      <x>" << line.end.x << "</x>\n";
        outFile << "      <y>" << line.end.y << "</y>\n";
        outFile << "    </vec2>\n";
        outFile << "    <vec3>\n"; // Fixed: Was </vec3>
        outFile << "      <x>" << line.color.x << "</x>\n";
        outFile << "      <y>" << line.color.y << "</y>\n";
        outFile << "      <z>" << line.color.z << "</z>\n";
        outFile << "    </vec3>\n";
        outFile << "  </line>\n";
    }

    for (const auto& box : boxes)
    {
        outFile << "  <box>\n";
        outFile << "    <vec2>\n";
        outFile << "      <x>" << box.minPos.x << "</x>\n";
        outFile << "      <y>" << box.minPos.y << "</y>\n";
        outFile << "    </vec2>\n";
        outFile << "    <vec2>\n";
        outFile << "      <x>" << box.maxPos.x << "</x>\n";
        outFile << "      <y>" << box.maxPos.y << "</y>\n";
        outFile << "    </vec2>\n";
        outFile << "    <vec3>\n"; // Fixed: Was </vec3>
        outFile << "      <x>" << box.color.x << "</x>\n";
        outFile << "      <y>" << box.color.y << "</y>\n";
        outFile << "      <z>" << box.color.z << "</z>\n";
        outFile << "    </vec3>\n";
        outFile << "  </box>\n";
    }

    for (const auto& pt : points)
    {
        outFile << "  <point>\n";
        outFile << "    <vec2>\n";
        outFile << "      <x>" << pt.pos.x << "</x>\n";
        outFile << "      <y>" << pt.pos.y << "</y>\n";
        outFile << "    </vec2>\n";
        outFile << "    <vec3>\n";
        outFile << "      <x>" << pt.color.x << "</x>\n";
        outFile << "      <y>" << pt.color.y << "</y>\n";
        outFile << "      <z>" << pt.color.z << "</z>\n";
        outFile << "    </vec3>\n";
        outFile << "  </point>\n";
    }

    outFile << "</layout>\n";
    outFile.close();
}