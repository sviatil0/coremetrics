#include "GUIFile.hpp"
#include "GUIElementFactory.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <typeinfo>

GUIFile::GUIFile() : manager(LayoutManager::getInstance())
{
}


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

    size_t tagEnd = source.find(">", start);
    if (tagEnd == std::string::npos) return "";
    start = tagEnd + 1;

    size_t end, tempEnd;
    if (tag == "layout")
    {
        tempEnd = source.find(closeTag, start);
        while (true)
        {
            size_t next = source.find(closeTag, tempEnd + closeTag.length());
            if (next == std::string::npos) break;
            tempEnd = next;
        }
        end = tempEnd;
    }
    else
    {
        end = source.find(closeTag, start);
    }
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
    if (v == "")
    {
        return vec2(0, 0);
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
    if (v == "")
    {
        return vec3(0, 0, 0);
    }
    size_t innerPos = 0;
    float x = std::stof(getContent(v, "x", innerPos));
    float y = std::stof(getContent(v, "y", innerPos));
    float z = std::stof(getContent(v, "z", innerPos));
    return vec3(x, y, z);
}

float GUIFile::extractFloat(const std::string& block, const std::string& key)
{
    size_t pos = block.find(key + "=");
    if (pos == std::string::npos) return 0.0f;

    pos += key.length() + 1;

    size_t end = block.find_first_of(" >", pos);
    
    return std::stof(block.substr(pos, end - pos));
}

Layout GUIFile::parseLayout(const std::string &block)
{
    // assuming no spaces bewteen tag and = and value (or between tag and >)
    float sx = extractFloat(block, "sX");
    float sy = extractFloat(block, "sY");
    float ex = extractFloat(block, "eX");
    float ey = extractFloat(block, "eY");
    size_t activePos = block.find("true");
    bool active = (activePos == std::string::npos) ? false : true;
    
    return Layout(vec2(sx, sy), vec2(ex, ey), active);
}

Tree<Layout>* GUIFile::recurseLayout(const std::string& block, Tree<Layout>* parent, int i)
{
    Layout currLayout = parseLayout(block);
    Tree<Layout>* node = manager.addChild(parent, std::move(currLayout));

    size_t elemPos;
    std::vector<GUIElementType> typeVec = {GUIElementType::POINT, GUIElementType::LINE, GUIElementType::BOX};

    // parse nested layouts
    std::string workingBlock = block;
    size_t layoutPos = 0;
    while (true)
    {
        size_t startPos = workingBlock.find("<layout", layoutPos);
        std::string childBlock = getContent(workingBlock, "layout", layoutPos);
        if (childBlock == "") 
        {
            break;
        }

        recurseLayout(childBlock, node, i+1);

        workingBlock.erase(startPos, layoutPos - startPos);
        layoutPos = startPos;
    }

    // parse elements
    for (GUIElementType &elemType : typeVec)
    {
        elemPos = 0;
        while (true)
        {
            std::string innerBlock;
            switch (elemType)
            {
                case GUIElementType::POINT:
                    innerBlock = getContent(workingBlock, "point", elemPos);
                    break;
                case GUIElementType::LINE:
                    innerBlock = getContent(workingBlock, "line", elemPos);
                    break;
                case GUIElementType::BOX:
                    innerBlock = getContent(workingBlock, "box", elemPos);
                    break;
            }
            if (innerBlock == "") break;

            size_t vecPos = 0;
            vec2 pos1 = parseVec2(innerBlock, vecPos);
            vec2 pos2 = parseVec2(innerBlock, vecPos);
            vec3 color = parseVec3(innerBlock, vecPos);
            std::unique_ptr<GUIElement> elem = GUIElementFactory::create(elemType, pos1, pos2, color);
            node->getData().addElement(std::move(elem));
        }
    }

    return node;
}

void GUIFile::readFile(const std::string &fileName)
{
    manager.clear();

    std::ifstream fp{fileName};
    if (!(fp))
    {
        std::cout << "File " << fileName << " cannot be opened\n";
        return;
    }

    std::stringstream buffer;
    buffer << fp.rdbuf();
    std::string content = buffer.str();
    size_t currPos = 0;
    
    std::stack<Tree<Layout>*> layoutStack;
    layoutStack.push(&manager.getRoot());
    Tree<Layout> *root = &manager.getRoot();

    // for each layout, store internal layouts and elements
    while (true)
    {
        std::string layoutBlock = getContent(content, "layout", currPos);
        if (layoutBlock == "") break;
        
        recurseLayout(layoutBlock, root, 0);
    }

}

static void writeNode(std::ofstream& outFile, const Tree<Layout>& node, int depth)
{
    const Layout& layout = node.getData();
    std::string indent(depth * 2, ' ');
    std::string inner(depth * 2 + 2, ' ');

    outFile << indent << "<layout>"
            << "sX=" << layout.getStart().x
            << " sY=" << layout.getStart().y
            << " eX=" << layout.getEnd().x
            << " eY=" << layout.getEnd().y
            << " active=" << (layout.isActive() ? "true" : "false") << "\n";

    for (const auto& elemPtr : layout.elements)
    {
        if (const Line* line = dynamic_cast<const Line*>(elemPtr.get()))
        {
            outFile << inner << "<line>\n";
            outFile << inner << "  <vec2><x>" << line->start.x << "</x><y>" << line->start.y << "</y></vec2>\n";
            outFile << inner << "  <vec2><x>" << line->end.x << "</x><y>" << line->end.y << "</y></vec2>\n";
            outFile << inner << "  <vec3><x>" << line->color.x << "</x><y>" << line->color.y << "</y><z>" << line->color.z << "</z></vec3>\n";
            outFile << inner << "</line>\n";
        }
        else if (const Box* box = dynamic_cast<const Box*>(elemPtr.get()))
        {
            outFile << inner << "<box>\n";
            outFile << inner << "  <vec2><x>" << box->minPos.x << "</x><y>" << box->minPos.y << "</y></vec2>\n";
            outFile << inner << "  <vec2><x>" << box->maxPos.x << "</x><y>" << box->maxPos.y << "</y></vec2>\n";
            outFile << inner << "  <vec3><x>" << box->color.x << "</x><y>" << box->color.y << "</y><z>" << box->color.z << "</z></vec3>\n";
            outFile << inner << "</box>\n";
        }
        else if (const Point* pt = dynamic_cast<const Point*>(elemPtr.get()))
        {
            outFile << inner << "<point>\n";
            outFile << inner << "  <vec2><x>" << pt->pos.x << "</x><y>" << pt->pos.y << "</y></vec2>\n";
            outFile << inner << "  <vec3><x>" << pt->color.x << "</x><y>" << pt->color.y << "</y><z>" << pt->color.z << "</z></vec3>\n";
            outFile << inner << "</point>\n";
        }
    }

    for (const auto& child : node.getChildren())
    {
        writeNode(outFile, *child, depth + 1);
    }

    outFile << indent << "</layout>\n";
}

void GUIFile::writeFile(std::string fileName)
{
    std::ofstream outFile{fileName};

    for (const auto& child : manager.getRoot().getChildren())
    {
        writeNode(outFile, *child, 0);
    }

    outFile.close();
}
