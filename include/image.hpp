#ifndef __IMAGE_HPP__
#define __IMAGE_HPP__

#include "GUIElement.hpp"
#include "linear.hpp" 
#include <string>

class Image : public GUIElement
{
private:
    std::string m_filePath;
    ivec2 m_position;

public:
    Image(std::string filePath, ivec2 position);
    virtual ~Image() = default;
    virtual void draw(Screen& screen) override;

    std::string getFilePath() const;
};

#endif