#ifndef __GUIFILE_HPP__
#define __GUIFILE_HPP__
#include <stdlib.h>
#include <vector>
#include <map>
#include <stack>
#include <string>
#include <fstream>
#include "GUIElements.hpp"
#include "GUIElementFactory.hpp"
#include "Tree.hpp"
#include "Layout.hpp"
#include "LayoutManager.hpp"

class GUIFile
{
private:
    LayoutManager &manager;

    std::string getContent(const std::string &source, const std::string &tag, size_t &pos);
    float extractFloat(const std::string& block, const std::string& key);
    vec2 parseVec2(const std::string &block, size_t &p);
    vec3 parseVec3(const std::string &block, size_t &p);
    Layout parseLayout(const std::string &block);
    Tree<Layout>* recurseLayout(const std::string& block, Tree<Layout>* parent, int i);

public:
    GUIFile();
    //file reading/parsing and writing
    void readFile(const std::string &fileName);
    void writeFile(std::string fileName);
};
#endif