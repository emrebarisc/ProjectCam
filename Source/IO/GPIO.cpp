#include "GPIO.h"

#include <stdexcept>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

#include "Globals.h"

GPIO::GPIO(bool use_bl) : useBl_(use_bl)
{
    const char* chip_path = "/dev/gpiochip0";
    int chip = open(chip_path, O_RDONLY | O_CLOEXEC);
    if (chip < 0) throw std::runtime_error("Failed to open /dev/gpiochip0");

    gpio_v2_line_request request{};
    request.num_lines = use_bl ? 3 : 2;
    request.offsets[0] = GPIO_DC;
    request.offsets[1] = GPIO_RST;
    if (use_bl) request.offsets[2] = GPIO_BL;
    std::snprintf(request.consumer, sizeof(request.consumer), "display-st7789");
    request.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    request.config.num_attrs = 1;
    request.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
    request.config.attrs[0].attr.values = (1ULL << 1); // RST high initially
    request.config.attrs[0].mask = use_bl ? 0b111 : 0b011;

    int result = ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &request);
    int err = errno;
    close(chip);
    if (result < 0)
    {
        errno = err;
        throw std::runtime_error("Failed to request GPIO lines for ST7789 display");
    }
    lineFd_ = request.fd;
}

GPIO::~GPIO()
{
    bl(false);
    if (lineFd_ >= 0) close(lineFd_);
}

void GPIO::DC(bool high)
{
    Set(0, high);
}

void GPIO::Reset(bool high)
{
    Set(1, high);
}

void GPIO::bl(bool high)
{
    if (useBl_)
    {
        Set(2, high);
    }
}

void GPIO::Set(int index, bool high)
{
    gpio_v2_line_values value{};
    value.mask = 1ULL << index;
    value.bits = high ? value.mask : 0;
    if (ioctl(lineFd_, GPIO_V2_LINE_SET_VALUES_IOCTL, &value) < 0)
    {
        throw std::runtime_error("Failed to set GPIO value");
    }
}