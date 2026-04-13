#ifndef __TREE_HPP__
#define __TREE_HPP__

#include <vector>
#include <memory>

template<typename T>
class Tree
{
private:
    T data;
    Tree<T>* parent;
    std::vector<std::unique_ptr<Tree<T>>> children;

public:
    Tree(T data) : data(std::move(data)), parent(nullptr)
    {
    }

    T& getData()
    {
        return data;
    }

    const T& getData() const
    {
        return data;
    }

    Tree<T>* getParent()
    {
        return parent;
    }

    bool isRoot() const
    {
        return parent == nullptr;
    }

    bool isLeaf() const
    {
        return children.empty();
    }

    std::vector<std::unique_ptr<Tree<T>>>& getChildren()
    {
        return children;
    }

    const std::vector<std::unique_ptr<Tree<T>>>& getChildren() const
    {
        return children;
    }

    Tree<T>* addChild(T&& childData)
    {
        children.push_back(std::make_unique<Tree<T>>(std::move(childData)));
        children.back()->parent = this;
        return children.back().get();
    }
};

#endif
