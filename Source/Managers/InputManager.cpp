#include "InputManager.h"

#include <iostream>

void InputManager::Init()
{
    listeners_.emplace_back(std::make_shared<ButtonListener>(
        5,
        [] {
            std::cout << "OK Single click" << std::endl;
        },
        [] {
            std::cout << "OK Double click" << std::endl;
        },
        [] {
            std::cout << "OK Hold" << std::endl;
        }));

    listeners_.emplace_back(std::make_shared<ButtonListener>(
        6,
        [] {
            std::cout << "Up Single click" << std::endl;
        },
        [] {
            std::cout << "Up Double click" << std::endl;
        },
        [] {
            std::cout << "Up Hold" << std::endl;
        }));

    listeners_.emplace_back(std::make_shared<ButtonListener>(
        13,
        [] {
            std::cout << "Down Single click" << std::endl;
        },
        [] {
            std::cout << "Down Double click" << std::endl;
        },
        [] {
            std::cout << "Down Hold" << std::endl;
        }));
}