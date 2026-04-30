#include "GUIFile.hpp"
#include "GUIElementFactory.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <array>
#include <cctype>
#include <locale>
#include <typeinfo>

std::string GUIFile::getContent(const std::string &source, const std::string &tag, size_t &pos)
{
    // attributes contained within opening tag for layouts, so ignore closing >
    std::string openTag = "<" + tag;
    std::string closeTag = "</" + tag + ">";

    size_t start = source.find(openTag, pos);
    // if tag not found in string, return
    if (start == std::string::npos) return "";
    
    start += openTag.length() + 1;

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

    std::string nameTag = "name";
    size_t nameStart = block.find(nameTag);
    std::string name = "";
    if (nameStart != std::string::npos)
    {
        nameStart += nameTag.length() + 1;
        size_t nameEnd = nameStart;
        while ((nameEnd < block.length()) && (block[nameEnd] != '>') && (block[nameEnd != ' ']))
        {
            nameEnd++;
        }
        name = block.substr(nameStart, nameEnd - nameStart);
    }
    
    return Layout(vec2(sx, sy), vec2(ex, ey), active, name);
}

Tree<Layout>* GUIFile::recurseLayout(const std::string& block, Tree<Layout>* parent, int i, LayoutManager& manager)
{
    Layout currLayout = parseLayout(block);
    Tree<Layout>* node = manager.addChild(parent, std::move(currLayout));

    size_t elemPos;
    std::array<GUIElementType, 8> types = {GUIElementType::POINT, GUIElementType::LINE, GUIElementType::BOX,
                                            GUIElementType::BUTTON, GUIElementType::LABEL,
                                            GUIElementType::BAR, GUIElementType::ROW, GUIElementType::IMAGE};

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

        recurseLayout(childBlock, node, i+1, manager);

        workingBlock.erase(startPos, layoutPos - startPos);
        layoutPos = startPos;
    }

    // parse elements
    for (auto itr = types.begin(); itr != types.end(); ++itr)
    {
        GUIElementType &elemType = *itr;
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
                case GUIElementType::LABEL:
                    innerBlock = getContent(workingBlock, "label", elemPos);
                    break;
                case GUIElementType::BUTTON:
                    innerBlock = getContent(workingBlock, "button", elemPos);
                    break;
                case GUIElementType::BAR:
                    innerBlock = getContent(workingBlock, "bar", elemPos);
                    break;
                case GUIElementType::ROW:
                    innerBlock = getContent(workingBlock, "row", elemPos);
                    break;
                case GUIElementType::IMAGE:
                    innerBlock = getContent(workingBlock, "image", elemPos);
                    break;
                default:
                    std::cerr << "unrecogized type\n";
                    break;
            }
            if (innerBlock == "") break;

            size_t vecPos = 0;
            std::unique_ptr<GUIElement> elem;

            if (elemType == GUIElementType::LABEL)
            {
                vec2 pos1 = parseVec2(innerBlock, vecPos);
                vec3 color = parseVec3(innerBlock, vecPos);
                std::string text = getContent(innerBlock, "text", vecPos);
                elem = GUIElementFactory::createLabel(text, pos1, color);
            }
            else if (elemType == GUIElementType::BUTTON)
            {
                vec2 pos1 = parseVec2(innerBlock, vecPos);
                vec2 pos2 = parseVec2(innerBlock, vecPos);
                vec3 color = parseVec3(innerBlock, vecPos);
                std::string sound = getContent(innerBlock, "sound", vecPos);
                std::string target = getContent(innerBlock, "target", vecPos);
                std::string hide = getContent(innerBlock, "hide", vecPos);
                elem = GUIElementFactory::createButton(pos1, pos2, color, sound, target, hide);
            }
            else if (elemType == GUIElementType::BAR)
            {
                float minVal = extractFloat(innerBlock, "min");
                float maxVal = extractFloat(innerBlock, "max");
                if (maxVal <= 0.0f) maxVal = 100.0f;
                vec2 pos1 = parseVec2(innerBlock, vecPos);
                vec2 pos2 = parseVec2(innerBlock, vecPos);
                vec3 fillColor = parseVec3(innerBlock, vecPos);
                vec3 bgColor = parseVec3(innerBlock, vecPos);
                size_t metricPos = 0;
                std::string metric = getContent(innerBlock, "metric", metricPos);
                elem = GUIElementFactory::createBar(
                    ivec2(static_cast<int>(pos1.x), static_cast<int>(pos1.y)),
                    ivec2(static_cast<int>(pos2.x), static_cast<int>(pos2.y)),
                    fillColor, bgColor, minVal, maxVal, metric);
            }
            else if (elemType == GUIElementType::ROW)
            {
                vec2 pos1 = parseVec2(innerBlock, vecPos);
                vec2 pos2 = parseVec2(innerBlock, vecPos);
                std::vector<std::string> cells;
                std::vector<float> weights;
                size_t cellSearch = 0;
                while (true)
                {
                    size_t cellTagStart = innerBlock.find("<cell", cellSearch);
                    if (cellTagStart == std::string::npos) break;
                    size_t headerEnd = innerBlock.find('>', cellTagStart);
                    if (headerEnd == std::string::npos) break;
                    std::string header = innerBlock.substr(cellTagStart, headerEnd - cellTagStart + 1);
                    float weight = extractFloat(header, "weight");
                    size_t closeStart = innerBlock.find("</cell>", headerEnd);
                    if (closeStart == std::string::npos) break;
                    std::string text = innerBlock.substr(headerEnd + 1, closeStart - headerEnd - 1);
                    cells.push_back(text);
                    weights.push_back(weight);
                    cellSearch = closeStart + std::string("</cell>").length();
                }
                vec3 color = parseVec3(innerBlock, vecPos);
                elem = GUIElementFactory::createRow(
                    ivec2(static_cast<int>(pos1.x), static_cast<int>(pos1.y)),
                    ivec2(static_cast<int>(pos2.x), static_cast<int>(pos2.y)),
                    std::move(cells), std::move(weights), color);
            }
            else if (elemType == GUIElementType::IMAGE)
            {
                vec2 pos = parseVec2(innerBlock, vecPos);
                size_t pathPos = 0;
                std::string path = getContent(innerBlock, "path", pathPos);
                if (!path.empty())
                {
                    elem = std::make_unique<Image>(path,
                        ivec2(static_cast<int>(pos.x), static_cast<int>(pos.y)));
                }
            }
            else
            {
                vec2 pos1 = parseVec2(innerBlock, vecPos);
                vec2 pos2 = parseVec2(innerBlock, vecPos);
                vec3 color = parseVec3(innerBlock, vecPos);
                elem = GUIElementFactory::create(elemType, pos1, pos2, color);
            }
            if (elem)
            {
                node->getData().addElement(std::move(elem));
            }
        }
    }

    return node;
}

void GUIFile::readFile(const std::string &fileName, LayoutManager& manager)
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

        recurseLayout(layoutBlock, root, 0, manager);
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

void GUIFile::writeFile(std::string fileName, LayoutManager& manager)
{
    std::ofstream outFile{fileName};

    for (const auto& child : manager.getRoot().getChildren())
    {
        writeNode(outFile, *child, 0);
    }

    outFile.close();
}
