#include "Display.h"

#include <algorithm>
#include <cstdint>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <stdexcept>

extern std::atomic<bool> isRunning;

#include "Globals.h"

void Display::Msleep(unsigned int ms)
{
    usleep(ms * 1000);
}

void Display::ScaleCameraFrameToDisplay(const uint8_t* rgb888, std::vector<uint8_t>& output, int sourceWidth, int sourceHeight)
{
    output.assign(WIDTH * HEIGHT * 3, 0);

    float scaleX = static_cast<float>(WIDTH) / sourceWidth;
    float scaleY = static_cast<float>(HEIGHT) / sourceHeight;
    float scale = std::min(scaleX, scaleY);

    int targetW = static_cast<int>(sourceWidth * scale);
    int targetH = static_cast<int>(sourceHeight * scale);

    int offsetX = (WIDTH - targetW) * 0.5f;
    int offsetY = (HEIGHT - targetH) * 0.5f;

    for (int y = 0; y < targetH; ++y)
    {
        int sy = (y * sourceHeight) / targetH;
        for (int x = 0; x < targetW; ++x)
        {
            int sx = (x * sourceWidth) / targetW;
            const uint8_t* p = &rgb888[(static_cast<size_t>(sy) * sourceWidth + sx) * 3];

            size_t i = (static_cast<size_t>(y + offsetY) * WIDTH + (x + offsetX)) * 3;
            output[i]     = p[0];
            output[i + 1] = p[1];
            output[i + 2] = p[2];
        }
    }
}

void Display::RGB888ToRGB565(const std::vector<uint8_t>& rgb888, std::vector<uint8_t>& output)
{
    output.assign(WIDTH * HEIGHT * 2, 0);

    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            const uint8_t* p = &rgb888[(y * WIDTH + x) * 3];
            const uint16_t pixel = (uint16_t(p[0] & 0xF8) << 8) |
                                   (uint16_t(p[1] & 0xFC) << 3) |
                                   (uint16_t(p[2]) >> 3);

            size_t i = (static_cast<size_t>(y * WIDTH + x) * 2);
            output[i]     = static_cast<uint8_t>(pixel >> 8);
            output[i + 1] = static_cast<uint8_t>(pixel);
        }
    }
}

Display::Display() : cameraManager_(nullptr)
{
}

Display::Display(std::shared_ptr<CameraManager> cameraManager)
    : cameraManager_(cameraManager)
{
}

Display::~Display()
{
    if (gpio_)
    {
        gpio_->bl(false);
    }
    lcd_.reset();
    gpio_.reset();
}

void Display::Init()
{
    if (!cameraManager_)
    {
        throw std::runtime_error("Display: CameraManager is not initialized.");
    }
}

void Display::Start()
{
    gpio_ = std::make_unique<GPIO>(true);
    lcd_ = std::make_unique<ST7789>(*gpio_);

    const int sourceWidth = cameraManager_->GetCameraWidth();
    const int sourceHeight = cameraManager_->GetCameraHeight();

    std::vector<uint8_t> displayFrame;
    std::vector<uint8_t> displayFrameRGB565;

    while (isRunning)
    {
        std::shared_ptr<uint8_t[]> frameData = cameraManager_->GetFrameDataArray();
        if (!frameData)
        {
            Msleep(10);
            continue;
        }

        ScaleCameraFrameToDisplay(frameData.get(), displayFrame, sourceWidth, sourceHeight);

        textRenderer_.DrawString(0, 16, "Selam canim!\nNaber?", Color(255, 255, 255, 255), Color(0, 0, 0, 255));
        textRenderer_.RenderOnto(displayFrame, WIDTH, HEIGHT, X_OFFSET, Y_OFFSET);

        RGB888ToRGB565(displayFrame, displayFrameRGB565);
        
        lcd_->Show(displayFrameRGB565);
    }
}

void Display::Shutdown()
{
    if (gpio_)
    {
        gpio_->bl(false);
    }
    
    lcd_.reset();
}