#ifndef __INPUT_MANAGER_H__
#define __INPUT_MANAGER_H__

#include "IManager.h"

#include <memory>
#include <vector>

#include "IO/ButtonListener.h"

class InputManager : public IManager
{
public:
    InputManager() = default;

    virtual void Init() override;

private:
    std::vector<std::shared_ptr<ButtonListener>> listeners_{};
};

#endif