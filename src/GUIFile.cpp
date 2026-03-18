#include "GUIFile.hpp"
#include <fstream>
#include <filesystem>
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

std::vector<std::string> GUIFile::splitString(const std::string &str, const std::string &delim)
{
    std::vector<std::string> tokens;
    std::size_t prev = 0;
    std::size_t pos = 0;
    //given set of delimiters as a string delim, search through string 
    //until first occurence of any delimiter is found, then update substring to search rest of string
    //continues until entire string has been processed/split by delims
    while ((pos = str.find_first_of(delim, prev)) != std::string::npos)
    {
        if (pos > prev)
        {
            tokens.push_back(str.substr(prev, pos - prev));
        }
        prev = pos + 1;
    }
    if (prev < str.length())
    {
        tokens.push_back(str.substr(prev, std::string::npos));
    }
    return tokens;
}

void GUIFile::readFile(std::string fileName)
{
    std::ifstream fp{fileName};
    if (!(fp))
    {
        return;
    }
    //while reading file, split each line by < and > symbols to interpret what is supposed to be added to the object
    //parse all data in the file into tokens vector, split by the < and > chars
    std::string data;
    std::vector<std::string> curLine;
    while (fp >> data)
    {
        curLine = splitString(data, "<>");
        tokens.insert(tokens.end(), curLine.begin(), curLine.end());
    }
    
    //declare/initialize temporary containers to hold whatever is currently being read
    std::map<std::string, float> fVec = {{"x", -1}, {"y", -1}, {"z", -1}};
    std::map<std::string, int> iVec = {{"x", -1}, {"y", -1}, {"z", -1}};
    size_t vecCount = 0, colorVec = 0;
    float fNum;
    int iNum;
    Line tempContainer;

    //treat tags as scopes, track them using a stack
    //to use a switch case, make enum of possible modes/identifier tags
    enum modeTypes {START, LAYOUT, LINE, BOX, POINT, VEC2, VEC3, IVEC2, IVEC3, X, Y, Z, IX, IY, IZ,
                    END_LAYOUT, END_LINE, END_BOX, END_POINT, END_VEC2, END_VEC3, END_IVEC2, END_IVEC3,
                    END_X, END_Y, END_Z};
    std::map<std::string, modeTypes> modeMap = {{"layout", LAYOUT}, {"line", LINE}, {"box", BOX}, {"point", POINT}, 
                    {"vec2", VEC2}, {"vec3", VEC3}, {"ivec2", IVEC2}, {"ivec3", IVEC3}, {"x", X}, {"y", Y}, {"z", Z},
                    {"/layout", END_LAYOUT}, {"/line", END_LINE}, {"/box", END_BOX}, {"/point", END_POINT}, 
                    {"/vec2", END_VEC2}, {"/vec3", END_VEC3}, {"/ivec2", END_IVEC2}, {"/ivec3", END_IVEC3},
                    {"/x", END_X}, {"/y", END_Y}, {"/z", END_Z}};
    std::stack<modeTypes> modes;
    modes.push(START);
    modeTypes nextMode;
    for (auto token : tokens)
    {
        nextMode = modeMap[token];
        switch (modes.top()) 
        {
            case START:
                if (token != "layout")
                {
                    return;
                }
                modes.push(LAYOUT);
                break;
            case LAYOUT:
                switch (nextMode)
                {
                    case LINE:
                    case BOX:
                    case POINT:
                        modes.push(nextMode);
                        break;
                    case END_LAYOUT:
                        modes.pop();
                        modes.push(START);
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case LINE:
                switch (nextMode)
                {
                    case VEC2:
                    case IVEC2:
                        if (vecCount < 2)
                        {
                            vecCount++;
                        } else
                        {
                            return;
                        }
                        modes.push(nextMode);
                        break;
                    case VEC3:
                    case IVEC3:
                        if (colorVec == 0)
                        {
                            colorVec = 1;
                        } else 
                        {
                            return;
                        }
                        modes.push(nextMode);
                        break;
                    case END_LINE:
                        lines.push_back(Line{tempContainer.start, tempContainer.end, tempContainer.color});
                        modes.pop();
                        vecCount = 0;
                        colorVec = 0;
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case BOX:
                switch (nextMode)
                {
                    case VEC2:
                    case IVEC2:
                        if (vecCount < 2)
                        {
                            vecCount++;
                        } 
                        else
                        {
                            return;
                        }
                        modes.push(nextMode);
                        break;
                    case VEC3:
                    case IVEC3:
                        if (colorVec == 0)
                        {
                            colorVec = 1;
                        } 
                        else 
                        {
                            return;
                        }
                        modes.push(nextMode);
                        break;
                    case END_BOX:
                        boxes.push_back(Box{tempContainer.start, tempContainer.end, tempContainer.color});
                        modes.pop();
                        vecCount = 0;
                        colorVec = 0;
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case POINT:
                switch (nextMode)
                {
                    case VEC2:
                    case IVEC2:
                        if (vecCount < 1)
                        {
                            vecCount++;
                        } 
                        else
                        {
                            return;
                        }
                        modes.push(nextMode);
                        break;
                    case VEC3:
                    case IVEC3:
                        if (colorVec == 0)
                        {
                            colorVec = 1;
                        } 
                        else 
                        {
                            return;
                        }
                        modes.push(nextMode);
                        break;
                    case END_POINT:
                        points.push_back(Point{tempContainer.start, tempContainer.color});
                        modes.pop();
                        vecCount = 0;
                        colorVec = 0;
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case VEC2:
                switch (nextMode)
                {
                    case X:
                    case Y:
                        if (fVec[token] != -1)
                        {
                            return;
                        }
                        modes.push(nextMode);
                        break;
                    case END_X:
                    case END_Y:
                        break;
                    case END_VEC2:
                        if (vecCount == 1)
                        {
                            tempContainer.start = vec2(fVec["x"], fVec["y"]);
                        }
                        else
                        {
                            tempContainer.end = vec2(iVec["x"], fVec["y"]);
                        }
                        fVec = {{"x", -1}, {"y", -1}, {"z", -1}};
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case VEC3:
                switch (nextMode)
                {
                    case X:
                    case Y:
                    case Z:
                        if (fVec[token] != -1)
                        {
                            return;
                        }
                        modes.push(nextMode);
                        break;
                    case END_X:
                    case END_Y:
                    case END_Z:
                        break;
                    case END_VEC3:
                        tempContainer.color = vec3(fVec["x"], fVec["y"], fVec["z"]);
                        fVec = {{"x", -1}, {"y", -1}, {"z", -1}};
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case IVEC2:
                switch (nextMode)
                {
                    case X:
                        if (iVec[token] != -1)
                        {
                            return;
                        }
                        modes.push(IX);
                        break;
                    case Y:
                        if (iVec[token] != -1)
                        {
                            return;
                        }
                        modes.push(IY);
                        break;
                    case END_X:
                    case END_Y:
                        break;
                    case END_IVEC2:
                        if (vecCount == 1)
                        {
                            tempContainer.start = vec2(iVec["x"], iVec["y"]);
                        }
                        else
                        {
                            tempContainer.end = vec2(iVec["x"], iVec["y"]);
                        }
                        iVec = {{"x", -1}, {"y", -1}, {"z", -1}};
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case IVEC3:
                switch (nextMode)
                {
                    case X:
                        if (iVec[token] != -1)
                        {
                            return;
                        }
                        modes.push(IX);
                        break;
                    case Y:
                        if (iVec[token] != -1)
                        {
                            return;
                        }
                        modes.push(IY);
                        break;
                    case Z:
                        if (iVec[token] != -1)
                        {
                            return;
                        }
                        modes.push(IZ);
                        break;
                    case END_X:
                    case END_Y:
                    case END_Z:
                        break;
                    case END_IVEC3:
                        tempContainer.color = vec3(iVec["x"], iVec["y"], iVec["z"]);
                        iVec = {{"x", -1}, {"y", -1}, {"z", -1}};
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case X:
                try
                {
                    fNum = std::stof(token);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                fVec["x"] = fNum;
                modes.pop();
                break;
            case Y:
                try
                {
                    fNum = std::stof(token);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                fVec["y"] = fNum;
                modes.pop();
                break;
            case Z:
                try
                {
                    fNum = std::stof(token);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                fVec["z"] = fNum;
                modes.pop();
                break;
            case IX:
                try
                {
                    iNum = std::stoi(token);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                iVec["x"] = iNum;
                modes.pop();
                break;
            case IY:
                try
                {
                    iNum = std::stoi(token);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                iVec["y"] = iNum;
                modes.pop();
                break;
            case IZ:
                try
                {
                    iNum = std::stoi(token);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                iVec["z"] = iNum;
                modes.pop();
                break;
            default:
                return;

        }
    }
    fp.close();
}

void GUIFile::writeFile(std::string fileName)
{
    std::ofstream outFile{fileName};
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