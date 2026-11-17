#include "InputManager.h"

#include <iostream>

#include "IO/ButtonListener.h"

void InputManager::Init()
{
    // OK button - BCM GPIO5
    ButtonListener okButton(
        5,
        [] {
            std::cout << "OK Single click" << std::endl;
        },
        [] {
            std::cout << "OK Double click" << std::endl;
        },
        [] {
            std::cout << "OK Hold" << std::endl;
        });

    // UP button - BCM GPIO6
    ButtonListener upButton(
        6,
        [] {
            std::cout << "Up Single click" << std::endl;
        },
        [] {
            std::cout << "Up Double click" << std::endl;
        },
        [] {
            std::cout << "Up Hold" << std::endl;
        });

    // DOWN button - BCM GPIO13
    ButtonListener downButton(
        13,
        [] {
            std::cout << "Down Single click" << std::endl;
        },
        [] {
            std::cout << "Down Double click" << std::endl;
        },
        [] {
            std::cout << "Down Hold" << std::endl;
        });
}