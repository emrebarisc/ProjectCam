#ifndef __DISPLAY_MANAGER_H__
#define __DISPLAY_MANAGER_H__

#include <memory>
#include <vector>
#include <cstdint>

#include "Managers/CameraManager.h"
#include "TextRenderer.h"
#include "GPIO.h"
#include "ST7789.h"

class Display
{
public:
    Display();
    explicit Display(std::shared_ptr<CameraManager> cameraManager);
    virtual ~Display();

    virtual void Init();
    void Start();

    void Shutdown();

private:
    void Msleep(unsigned int ms);
    void ScaleCameraFrameToDisplay(const uint8_t* rgb888, std::vector<uint8_t>& output, int sourceWidth, int sourceHeight);
    void RGB888ToRGB565(const std::vector<uint8_t>& rgb888, std::vector<uint8_t>& output);

    std::shared_ptr<CameraManager> cameraManager_;
    TextRenderer textRenderer_;
    
    std::unique_ptr<GPIO> gpio_;
    std::unique_ptr<ST7789> lcd_;
};

#endif