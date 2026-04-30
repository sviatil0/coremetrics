#include "LayoutUtils.hpp"

Bar *findBarByMetric(Tree<Layout> &node, const std::string &metric)
{
    for (const auto &element : node.getData().elements)
    {
        Bar *bar = dynamic_cast<Bar *>(element.get());
        if (bar != nullptr && bar->getMetricName() == metric)
        {
            return bar;
        }
    }
    for (auto &child : node.getChildren())
    {
        Bar *bar = findBarByMetric(*child, metric);
        if (bar != nullptr)
        {
            return bar;
        }
    }
    return nullptr;
}

Label *nthLabelInLayout(Tree<Layout> &layoutNode, std::size_t index)
{
    std::size_t seen = 0;
    for (const auto &element : layoutNode.getData().elements)
    {
        Label *label = dynamic_cast<Label *>(element.get());
        if (label == nullptr)
        {
            continue;
        }
        if (seen == index)
        {
            return label;
        }
        ++seen;
    }
    return nullptr;
}

void collectRows(Tree<Layout> &node, std::vector<Row *> &out)
{
    for (const auto &element : node.getData().elements)
    {
        Row *row = dynamic_cast<Row *>(element.get());
        if (row != nullptr)
        {
            out.push_back(row);
        }
    }
    for (auto &child : node.getChildren())
    {
        collectRows(*child, out);
    }
}
