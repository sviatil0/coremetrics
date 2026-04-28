#ifndef __LAYOUT_UTILS_HPP__
#define __LAYOUT_UTILS_HPP__

#include "Tree.hpp"
#include "Layout.hpp"
#include "Bar.hpp"
#include "Label.hpp"
#include "Row.hpp"

Bar *findBarByMetric(Tree<Layout> &node, const std::string &metric);
Label *nthLabelInLayout(Tree<Layout> &layoutNode, std::size_t index);
void collectRows(Tree<Layout> &node, std::vector<Row *> &out);

#endif
