#include "ST7789.h"

#include <algorithm>
#include <cerrno>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "Globals.h"

namespace
{
    void msleep(unsigned int ms)
    { usleep(ms * 1000); }
}

ST7789::ST7789(GPIO& gpio) : gpio_(gpio)
{
    spiFd_ = open("/dev/spidev0.0", O_RDWR | O_CLOEXEC);
    if (spiFd_ < 0) throw std::runtime_error("Failed to open /dev/spidev0.0");

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t hz = SPI_HZ; // Ensure SPI_HZ is visible via globals/headers
    if (ioctl(spiFd_, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(spiFd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(spiFd_, SPI_IOC_WR_MAX_SPEED_HZ, &hz) < 0)
        throw std::runtime_error("Failed to configure SPI");
    Init();
}

ST7789::~ST7789()
{
    try
    {
        CMD(0x28); // Display Off
        msleep(20);
        CMD(0x10); // Sleep In
        msleep(120);
    }
    catch (...)
    {
    }

    if (spiFd_ >= 0)
    {
        close(spiFd_);
    }
}

void ST7789::Show(const std::vector<uint8_t>& rgb565)
{
    if (rgb565.size() != static_cast<size_t>(WIDTH * HEIGHT * 2))
    {
        throw std::runtime_error("Incorrect framebuffer size for ST7789");
    }

    const int x0 = X_OFFSET, y0 = Y_OFFSET;
    const int x1 = x0 + WIDTH - 1, y1 = y0 + HEIGHT - 1;
    CMD(0x2A, {uint8_t(x0 >> 8), uint8_t(x0), uint8_t(x1 >> 8), uint8_t(x1)});
    CMD(0x2B, {uint8_t(y0 >> 8), uint8_t(y0), uint8_t(y1 >> 8), uint8_t(y1)});
    CMD(0x2C);
    gpio_.DC(true);
    WriteBytes(rgb565.data(), rgb565.size());
}

void ST7789::WriteBytes(const uint8_t* ptr, size_t len)
{
    while (len)
    {
        const size_t chunk = std::min<size_t>(len, 4096);
        const ssize_t n = ::write(spiFd_, ptr, chunk);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) throw std::runtime_error("SPI write failed");
        ptr += n;
        len -= static_cast<size_t>(n);
    }
}

void ST7789::CMD(uint8_t c, std::initializer_list<uint8_t> params)
{
    gpio_.DC(false);
    WriteBytes(&c, 1);
    if (params.size())
    {
        gpio_.DC(true);
        WriteBytes(params.begin(), params.size());
    }
}

void ST7789::Init()
{
    gpio_.bl(false);
    gpio_.Reset(true);  msleep(30);
    gpio_.Reset(false); msleep(50);
    gpio_.Reset(true);  msleep(150);

    CMD(0x01); msleep(150);
    CMD(0x11); msleep(120);
    CMD(0x36, {0xA0});
    CMD(0x3A, {0x55});
    CMD(0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33});
    CMD(0xB7, {0x35});
    CMD(0xBB, {0x19});
    CMD(0xC0, {0x2C});
    CMD(0xC2, {0x01});
    CMD(0xC3, {0x12});
    CMD(0xC4, {0x20});
    CMD(0xC6, {0x0F});
    CMD(0xD0, {0xA4, 0xA1});
    CMD(0xE0, {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F,
               0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23});
    CMD(0xE1, {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F,
               0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23});
    CMD(0x21);
    CMD(0x13);
    CMD(0x29); msleep(100);
    gpio_.bl(true);
}