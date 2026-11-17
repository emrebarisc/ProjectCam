#include "TextRenderer.h"

#include "Font.h"

TextRenderer::TextRenderer()
{
    frame_ = new uint8_t[frameWidth_ * frameHeight_ * colorChannel_];
}

TextRenderer::~TextRenderer()
{
    delete[] frame_;
}

void TextRenderer::DrawCharacter(int x, int y, uint8_t character, const Color& color)
{
    int scaledWidth = FONT_WIDTH * scale_;
    int scaledHeight = FONT_HEIGHT * scale_;

    for (int letterLine = 0; letterLine < scaledHeight; ++letterLine)
    {
        int targetY = y + letterLine;
        if (targetY < 0 || frameHeight_ <= targetY)
        {
            continue;
        }

        for (int pixelX = 0; pixelX < scaledWidth; ++pixelX)
        {
            int targetX = x + pixelX;
            if (targetX < 0 || frameWidth_ <= targetX)
            {
                continue;
            }
            
            int fontRowIndex = pixelX / scale_;
            if (FONT_WIDTH <= fontRowIndex)
            {
                continue;
            }

            int fontBitIndex = letterLine / scale_;
            if (FONT_HEIGHT <= fontBitIndex)
            {
                continue;
            }

            uint8_t columnBits = font_8x8[character - 32][fontRowIndex];

            if (0x01 & (columnBits >> fontBitIndex))
            {
                int frameIndex = (targetY * frameWidth_ + targetX) * colorChannel_;
                
                for (int c = 0; c < colorChannel_; ++c)
                {
                    frame_[frameIndex + c] = color[c];
                }
            }
        }
    }
}

void TextRenderer::DrawString(int x, int y, const std::string& text, const Color& color)
{
    int cursorX = x;
    int cursorY = y;
    for(char character : text)
    {
        if(character == '\n')
        {
            cursorX = x;
            cursorY += FONT_HEIGHT * scale_;
            continue;
        }

        DrawCharacter(cursorX, cursorY, character, color);
        cursorX += (FONT_WIDTH + FONT_SPACING) * scale_; 

        if(frameWidth_ < cursorX)
        {
            cursorX = x;
            cursorY += FONT_HEIGHT * scale_;
        }
    }
}

void TextRenderer::RenderOnto(
    std::vector<uint8_t>& subFrame, 
    int width, int height, 
    int xOffset, int yOffset)
{
    for (int y = 0; y < frameHeight_; ++y)
    {
        int destY = y + yOffset;
        
        if (destY < 0 || height <= destY)
        {
            continue;
        }

        for (int x = 0; x < frameWidth_; ++x)
        {
            int destX = x + xOffset;
            if (destX < 0 || destX >= width)
            {
                continue;
            }

            int thisFrameIndex = (y * frameWidth_ + x) * 4;
            
            uint8_t a = frame_[thisFrameIndex + 3];
            if (a == 0)
            {
                continue;
            }

            uint8_t r = frame_[thisFrameIndex];
            uint8_t g = frame_[thisFrameIndex + 1];
            uint8_t b = frame_[thisFrameIndex + 2];

            float aNormalized = a / 255.f;

            int subFrameIndex = (destY * width + destX) * 3;

            subFrame[subFrameIndex]     = 
                static_cast<uint8_t>(subFrame[subFrameIndex]     * (1.f - aNormalized) + r * aNormalized);
            subFrame[subFrameIndex + 1] = 
                static_cast<uint8_t>(subFrame[subFrameIndex + 1] * (1.f - aNormalized) + g * aNormalized);
            subFrame[subFrameIndex + 2] = 
                static_cast<uint8_t>(subFrame[subFrameIndex + 2] * (1.f - aNormalized) + b * aNormalized);
        }
    }
}