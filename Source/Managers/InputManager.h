#ifndef __INPUT_MANAGER_H__
#define __INPUT_MANAGER_H__

#include "IManager.h"

class InputManager : public IManager
{
public:
    InputManager() = default;

    virtual void Init() override;
};

#endif