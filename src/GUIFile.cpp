#include "GUIFile.hpp"

GUIFile::GUIFile() {} //do we need to do anything with the constructor?

void GUIFile::setPoint(Point point)
{

}
void GUIFile::setLine(Line line)
{

}
void GUIFile::setBox(Box box)
{

}

std::vector<Point> GUIFile::getPoints()
{

}
std::vector<Line> GUIFile::getLines()
{

}
std::vector<Box> GUIFile::getBoxes()
{

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

void GUIFile::readFile(std::string fileName);
{
    std::ifstream fp{fileName};
    if (!(fp))
    {
        return;
    }
    //while reading file, split each line by < and > symbols to interpret what is supposed to be read
    //store current mode to validate what can be read next
    //possible modes: {layout, line, box, point, vec2, vec3, ivec2, ivec3, x, y, z};
    
    //parse all data in the file into tokens vector, split by the < and > chars
    std::string data;
    std::vector<std::string> curLine;
    while (fp >> data)
    {
        curLine = splitString(data, "<>");
        tokens.insert(tokens.end(), curLine.begin(), curLine.end())
    }
    
    //declare/initialize temporary containers to hold whatever is currently being read
    std::map<std::string, float> fVec = {{"x", -1}, {"y", -1}, {"z", -1}};
    std::map<std::string, int> iVec = {{"x", -1}, {"y", -1}, {"z", -1}};
    size_t vecCount = 0, colorVec = 0;
    Line tempContainer;

    //treat tags as scopes, track them using a stack
    std::stack<std::string> modes;
    modes.push("start");
    for (auto token : tokens)
    {
        switch (modes.top()) 
        {
            case "start":
                if (token != "layout")
                {
                    return;
                }
                modes.push(token);
                break;
            case "layout":
                switch (token)
                {
                    case "line":
                    case "box":
                    case "point":
                        modes.push(token);
                        break;
                    case "/layout":
                        modes.pop();
                        modes.push("start");
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case "line":
                switch (token)
                {
                    case "vec2":
                    case "ivec2":
                        if (vecCount < 2)
                        {
                            vecCount++;
                        } else
                        {
                            return;
                        }
                        modes.push(token);
                        break;
                    case "vec3":
                    case "ivec3":
                        if (colorVec == 0)
                        {
                            colorVec = 1;
                        } else 
                        {
                            return;
                        }
                        modes.push(token);
                        break;
                    case "/line":
                        lines.push_back(Line{tempContainer.start, tempContainer.end, tempContainer.color});
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case "box":
                switch (token)
                {
                    case "vec2":
                    case "ivec2":
                        if (vecCount < 2)
                        {
                            vecCount++;
                        } else
                        {
                            return;
                        }
                        modes.push(token);
                        break;
                    case "vec3":
                    case "ivec3":
                        if (colorVec == 0)
                        {
                            colorVec = 1;
                        } else 
                        {
                            return;
                        }
                        modes.push(token);
                        break;
                    case "/box":
                        boxes.push_back(Box{tempContainer.start, tempContainer.end, tempContainer.color});
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case "point":
                switch (token)
                {
                    case "vec2":
                    case "ivec2":
                        if (vecCount < 1)
                        {
                            vecCount++;
                        } 
                        else
                        {
                            return;
                        }
                        modes.push(token);
                        break;
                    case "vec3":
                    case "ivec3":
                        if (colorVec == 0)
                        {
                            colorVec = 1;
                        } 
                        else 
                        {
                            return;
                        }
                        modes.push(token);
                        break;
                    case "/point":
                        points.push_back(Point{tempContainer.start, tempContainer.color});
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case "vec2":
                switch (token)
                {
                    case "x":
                    case "y":
                        if (fVec[token] != -1)
                        {
                            return;
                        }
                        modes.push(token);
                        break;
                    case "/x":
                    case "/y":
                        break;
                    case "/vec2":
                        if (vecCount == 1)
                        {
                            tempContainer.start = new vec2(fVec["x"], fVec["y"]);
                        }
                        else
                        {
                            tempContainer.end = new vec2(iVec["x"], fVec["y"]);
                        }
                        fVec = {{"x", -1}, {"y", -1}, {"z", -1}};
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case "vec3":
                switch (token)
                {
                    case "x":
                    case "y":
                    case "z":
                        if (fVec[token] != -1)
                        {
                            return;
                        }
                        modes.push(token);
                        break;
                    case "/x":
                    case "/y":
                    case "/z":
                        break;
                    case "/vec3":
                        tempContainer.color = new vec3(fVec["x"], fVec["y"], fVec["z"]);
                        fVec = {{"x", -1}, {"y", -1}, {"z", -1}};
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case "ivec2":
                switch (token)
                {
                    case "x":
                    case "y":
                        if (iVec[token] != -1)
                        {
                            return;
                        }
                        modes.push("i" + token);
                        break;
                    case "/x":
                    case "/y":
                        break;
                    case "/ivec2":
                        if (vecCount == 1)
                        {
                            tempContainer.start = new ivec2(iVec["x"], iVec["y"]);
                        }
                        else
                        {
                            tempContainer.end = new ivec2(iVec["x"], iVec["y"]);
                        }
                        iVec = {{"x", -1}, {"y", -1}, {"z", -1}};
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case "ivec3":
                switch (token)
                {
                    case "x":
                    case "y":
                    case "z":
                        if (iVec[token] != -1)
                        {
                            return;
                        }
                        modes.push("i" + token);
                        break;
                    case "/x":
                    case "/y":
                    case "/z":
                        break;
                    case "/ivec3":
                        tempContainer.color = new ivec3(iVec["x"], iVec["y"], iVec["z"]);
                        iVec = {{"x", -1}, {"y", -1}, {"z", -1}};
                        modes.pop();
                        break;
                    default:
                        return; //clear containers when invalid structure encountered?
                }
                break;
            case "x":
                try
                {
                    float num;
                    std::stof(token, num);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                fVec["x"] = num;
                break;
            case "y":
                try
                {
                    float num;
                    std::stof(token, num);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                fVec["y"] = num;
                break;
            case "z":
                try
                {
                    float num;
                    std::stof(token, num);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                fVec["z"] = num;
                break;
            case "ix":
                try
                {
                    float num;
                    std::stoi(token, num);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                iVec["x"] = num;
                break;
            case "iy":
                try
                {
                    float num;
                    std::stoi(token, num);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                iVec["y"] = num;
                break;
            case "iz":
                try
                {
                    float num;
                    std::stoi(token, num);
                } 
                catch (const std::invalid_argument &ia)
                {
                    return;
                }
                iVec["z"] = num;
                break;
            default:
                return;
            
        }
    }


    close(fp);
}
void GUIFile::writeFile(std::string fileName)
{
    if (!(std::filesystem::exists(fileName)))
    {
        return;
    }

}
