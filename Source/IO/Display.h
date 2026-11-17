#ifndef __DISPLAY_MANAGER_H__
#define __DISPLAY_MANAGER_H__

#include <memory>

#include "Managers/CameraManager.h"
#include "TextRenderer.h"

class Display
{
public:
    Display();
    explicit Display(std::shared_ptr<CameraManager> cameraManager);
    virtual ~Display() = default;

    virtual void Init();
    void Start();

private:
    std::shared_ptr<CameraManager> cameraManager_;
    TextRenderer textRenderer_;
};

#endif