#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdexcept>
#include <stdint.h>

class Color
{
public:
    Color() = default;
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    uint8_t operator[](int index) const
    {
        if(3 < index)
        {
            throw std::out_of_range("Color index is out of range!");
        }

        if(index == 0)
        {
            return r;
        }
        else if(index == 1)
        {
            return g;
        }
        else if(index == 2)
        {
            return b;
        }
        else
        {
            return a;
        }
    }

    uint8_t r{ 0 };
    uint8_t g{ 0 };
    uint8_t b{ 0 };
    uint8_t a{ 255 };
};

#endif