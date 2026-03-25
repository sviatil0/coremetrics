#include "GUIFile.hpp"
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
    std::string openTag = "<" + tag + ">";
    std::string closeTag = "</" + tag + ">";

    size_t start = source.find(openTag, pos);
    // if tag not found in string, return
    if (start == std::string::npos) return "";

    start += openTag.length();
    size_t end = source.find(closeTag, start);
    if (end == std::string::npos) return "";

    // update reference to pos and return content btwn tags
    pos = end + closeTag.length();
    return source.substr(start, end - start);
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

    std::ifstream fp{fileName};
    if (!(fp))
    {
        return;
    }

    std::stringstream buffer;
    buffer << fp.rdbuf();
    std::string content = buffer.str();
    size_t currPos = 0, elemPos = 0;

    // for each layout, store elements
    while (true)
    {
        std::string layoutBlock = getContent(content, "layout", currPos);
        if (layoutBlock == "") break;

        elemPos = 0;
        while (true)
        {
            std::string lineBlock = getContent(layoutBlock, "line", elemPos);
            if (lineBlock == "") break;

            size_t innerPos = 0;
            Line l;
            l.start = parseVec2(lineBlock, innerPos);
            l.end = parseVec2(lineBlock, innerPos);
            l.color = parseVec3(lineBlock, innerPos);
            lines.push_back(l);
            
        }

        elemPos = 0;
        while (true)
        {
            std::string boxBlock = getContent(layoutBlock, "box", elemPos);
            if (boxBlock == "") break;

            size_t innerPos = 0;
            Box b;
            b.minPos = parseVec2(boxBlock, innerPos);
            b.maxPos = parseVec2(boxBlock, innerPos);
            b.color = parseVec3(boxBlock, innerPos);
            boxes.push_back(b);
        }
        elemPos = 0;
        while (true)
        {
            std::string pointBlock = getContent(layoutBlock, "point", elemPos);
            if (pointBlock == "") break;

            size_t innerPos = 0;
            Point p;
            p.pos = parseVec2(pointBlock, innerPos);
            p.color = parseVec3(pointBlock, innerPos);
            points.push_back(p);
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
