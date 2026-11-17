#ifndef __ST7789_H__
#define __ST7789_H__

#include <cstdint>
#include <vector>
#include <initializer_list>
#include "GPIO.h"

class ST7789
{
public:
    explicit ST7789(GPIO& gpio);
    ~ST7789();

    ST7789(const ST7789&) = delete;
    ST7789& operator=(const ST7789&) = delete;

    void Show(const std::vector<uint8_t>& rgb565);

private:
    GPIO& gpio_;
    int spiFd_ = -1;

    void WriteBytes(const uint8_t* ptr, size_t len);
    void CMD(uint8_t c, std::initializer_list<uint8_t> params = {});
    void Init();
};

#endif