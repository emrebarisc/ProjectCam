#ifndef __TEXT_RENDERER_H__
#define __TEXT_RENDERER_H__

#include <string>
#include <vector>

#include "Color.h"

class TextRenderer
{
public: 
    TextRenderer();
    ~TextRenderer();

    void DrawCharacter(int x, int y, uint8_t character, const Color& color);
    void DrawString(int x, int y, const std::string& text, const Color& color);

    void RenderOnto(std::vector<uint8_t>& subFrame, int width, int height, int xOffset, int yOffset);

private:
    float scale_{ 2.f };
    int frameWidth_{ 280 };
    int frameHeight_{ 240 };
    int colorChannel_{ 4 };
    uint8_t* frame_;
};

#endif