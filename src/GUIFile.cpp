#include "GUIFile.hpp"
#include "Layout.hpp"
#include "LayoutManager.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>

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

std::string GUIFile::getContent(const std::string &source, const std::string &tag, size_t &pos)
{
    // attributes contained within opening tag for layouts, so ignore closing >
    std::string openTag = "<" + tag;
    std::string closeTag = "</" + tag + ">";

    size_t start = source.find(openTag, pos);
    // if tag not found in string, return
    if (start == std::string::npos) return "";

    start += openTag.length() + 1; 
    size_t end = source.find(closeTag, start);
    if (end == std::string::npos) return "";

    // update reference to pos and return content btwn tags
    pos = end + closeTag.length();
    return source.substr(start, end - start);
}

Layout GUIFile::parseLayout(const std::string &block)
{
    // assuming no spaces bewteen tag and = and value (or between tag and >)
    size_t startXPos = block.find("sX") + 1;
    size_t startYPos = block.find("sY") + 1;
    size_t endXPos = block.find("eX") + 1;
    size_t endYPos = block.find("eY") + 1;
    size_t activePos = block.find("true");

    float sx = std::stof(block.substr(startXPos));
    float sy = std::stof(block.substr(startYPos));
    float ex = std::stof(block.substr(endXPos));
    float ey = std::stof(block.substr(endYPos));
    bool active = (activePos == std::string::npos) ? false : true;
    
    return Layout(vec2(sx, sy), vec2(ex, ey), active);
}

vec2 GUIFile::parseVec2(const std::string &block, size_t &p)
{
    std::string v = getContent(block, "vec2", p);
    if (v == "")
    {
        v = getContent(block, "ivec2", p);
    }
    size_t innerPos = 0;
    float x = std::stof(getContent(v, "x", innerPos));
    float y = std::stof(getContent(v, "y", innerPos));
    return vec2(x, y);
}

vec3 GUIFile::parseVec3(const std::string &block, size_t &p)
{
    std::string v = getContent(block, "vec3", p);
    if (v == "")
    {
        v = getContent(block, "ivec3", p);
    }
    size_t innerPos = 0;
    float x = std::stof(getContent(v, "x", innerPos));
    float y = std::stof(getContent(v, "y", innerPos));
    float z = std::stof(getContent(v, "z", innerPos));
    return vec3(x, y, z);
}

void GUIFile::readFile(const std::string fileName)
{
    // clear containers before refilling them with file elements
    lines.clear();
    boxes.clear();
    points.clear();

    // update above lines to instead delete old layoutManager (if it exists) and create new one

    std::ifstream fp{fileName};
    if (!(fp))
    {
        return;
    }

    std::stringstream buffer;
    buffer << fp.rdbuf();
    std::string content = buffer.str();
    size_t currPos = 0, elemPos;
    // need vector of trees bc layouts don't have to be in same tree
    // what is layoutManager for then?


    // for each layout, store elements
    while (true)
    {
        std::string layoutBlock = getContent(content, "layout", currPos);
        if (layoutBlock == "") break;
        Layout currLayout = parseLayout(layoutBlock);

        elemPos = 0;
        while (true)
        {
            std::string lineBlock = getContent(layoutBlock, "line", elemPos);
            if (lineBlock == "") break;

            size_t vecPos = 0;
            Line l;
            l.start = parseVec2(lineBlock, vecPos);
            l.end = parseVec2(lineBlock, vecPos);
            l.color = parseVec3(lineBlock, vecPos);
            currLayout.addElement(l);
        }

        elemPos = 0;
        while (true)
        {
            std::string boxBlock = getContent(layoutBlock, "box", elemPos);
            if (boxBlock == "") break;

            size_t vecPos = 0;
            Box b;
            b.minPos = parseVec2(boxBlock, vecPos);
            b.maxPos = parseVec2(boxBlock, vecPos);
            b.color = parseVec3(boxBlock, vecPos);
            currLayout.addElement(b);
        }

        elemPos = 0;
        while (true)
        {
            std::string pointBlock = getContent(layoutBlock, "point", elemPos);
            if (pointBlock == "") break;

            size_t vecPos = 0;
            Point p;
            p.pos = parseVec2(pointBlock, vecPos);
            p.color = parseVec3(pointBlock, vecPos);
            currLayout.addElement(p);
        }
    }

}

void GUIFile::writeFile(std::string fileName)
{
    std::ofstream outFile{fileName};
    outFile << "<layout>\n";

    for (const auto &line : lines)
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

    for (const auto &box : boxes)
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

    for (const auto &pt : points)
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
