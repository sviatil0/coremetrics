#ifndef __SCREEN_HPP__
#define __SCREEN_HPP__

#include <SDL3/SDL.h>
#include "vec2.hpp"
#include "vec3.hpp"

class Screen
{
private:
    unsigned int width;
    unsigned int height;
    SDL_Surface* surface = nullptr;

public:
    Screen() = delete; //if no args passed to constructor, cannot create object
    Screen(unsigned int w, unsigned int h) : width(w), height(h), surface(nullptr)
    {
        if (width == 0 || height == 0)
        {
            std::cerr << "Dimensions must be >0" << '\n';
        }
        surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
        if (!surface)
        {
            std::cerr << "Failed to create surface: " << SDL_GetError() << '\n';
        };
    }

    ~Screen() 
    {
        if (surface) {
            SDL_DestroySurface(surface);
            surface = nullptr;
        }
    }

    
    //need templated functions (below) in order to provide support for both int and float vectors
    //also means whole implementation must be within header file (like for vec2 and vec3)
    template<typename T, typename U>
    void drawPixel(const vec2<T>& pos, const vec3<U>& color)
    {

    }

    template<typename T, typename U, typename V>
    void drawLine(const vec2<T>& a, const vec2<U>& b, const vec3<V>& color)
    {
        //Bresenham line implementation
    }

    template<typename T, typename U, typename V>
    void drawBox(const vec2<T>& a, const vec2<U>& b, const vec3<V>& color)
    {
        //determine which vec is min and which is max within method
    }

    void blitTo(SDL_Surface* target)
    {

    }

};

#endif