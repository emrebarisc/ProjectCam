#include "Display.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>

namespace
{
    constexpr int WIDTH = 280;
    constexpr int HEIGHT = 240;
    constexpr int X_OFFSET = 20;
    constexpr int Y_OFFSET = 0; 
    constexpr unsigned int GPIO_DC = 25;
    constexpr unsigned int GPIO_RST = 27;
    constexpr unsigned int GPIO_BL = 18;
    constexpr unsigned int SPI_HZ = 24000000;

    void msleep(unsigned int ms)
    { usleep(ms * 1000); }

    class GPIO
    {
    public:
        explicit GPIO(bool use_bl) : useBl_(use_bl)
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

        ~GPIO()
        { if (lineFd_ >= 0) close(lineFd_); }
        GPIO(const GPIO&) = delete;
        GPIO& operator=(const GPIO&) = delete;

        void DC(bool high)
        {
            Set(0, high);
        }
        void Reset(bool high)
        {
            Set(1, high);
        }
        void bl(bool high)
        {
            if (useBl_)
            {
                Set(2, high);
            }
        }

    private:
        int lineFd_ = -1;
        bool useBl_;

        void Set(int index, bool high)
        {
            gpio_v2_line_values value{};
            value.mask = 1ULL << index;
            value.bits = high ? value.mask : 0;
            if (ioctl(lineFd_, GPIO_V2_LINE_SET_VALUES_IOCTL, &value) < 0)
            {
                throw std::runtime_error("Failed to set GPIO value");
            }
        }
    };

    class ST7789
    {
    public:
        explicit ST7789(GPIO& gpio) : gpio_(gpio)
        {
            spiFd_ = open("/dev/spidev0.0", O_RDWR | O_CLOEXEC);
            if (spiFd_ < 0) throw std::runtime_error("Failed to open /dev/spidev0.0");

            uint8_t mode = SPI_MODE_0;
            uint8_t bits = 8;
            uint32_t hz = SPI_HZ;
            if (ioctl(spiFd_, SPI_IOC_WR_MODE, &mode) < 0 ||
                ioctl(spiFd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
                ioctl(spiFd_, SPI_IOC_WR_MAX_SPEED_HZ, &hz) < 0)
                throw std::runtime_error("Failed to configure SPI");
            Init();
        }

        ~ST7789()
        {
            if (spiFd_ >= 0)
            {
                close(spiFd_);
            }
        }

        ST7789(const ST7789&) = delete;
        ST7789& operator=(const ST7789&) = delete;

        void Show(const std::vector<uint8_t>& rgb565)
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

    private:
        GPIO& gpio_;
        int spiFd_ = -1;

        void WriteBytes(const uint8_t* ptr, size_t len)
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

        void CMD(uint8_t c, std::initializer_list<uint8_t> params = {})
        {
            gpio_.DC(false);
            WriteBytes(&c, 1);
            if (params.size())
            {
                gpio_.DC(true);
                WriteBytes(params.begin(), params.size());
            }
        }

        void Init()
        {
            gpio_.bl(false);
            gpio_.Reset(true);  msleep(30);
            gpio_.Reset(false); msleep(50);
            gpio_.Reset(true);  msleep(150);

            CMD(0x01); msleep(150);
            CMD(0x11); msleep(120);
            // MADCTL set to Landscape orientation (0x70 or 0xA0 depending on ribbon cable placement)
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
    };

    void ScaleCameraFrameToDisplay(const uint8_t* rgb888, std::vector<uint8_t>& output, int sourceWidth, int sourceHeight)
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

    void RGB888ToRGB565(std::vector<uint8_t>& rgb888, std::vector<uint8_t>& output)
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
}

Display::Display() : cameraManager_(nullptr)
{
}

Display::Display(std::shared_ptr<CameraManager> cameraManager)
    : cameraManager_(cameraManager)
{
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
    GPIO gpio(true);
    ST7789 lcd(gpio);

    const int sourceWidth = cameraManager_->GetCameraWidth();
    const int sourceHeight = cameraManager_->GetCameraHeight();

    std::vector<uint8_t> displayFrame;
    std::vector<uint8_t> displayFrameRGB565;

    while (true)
    {
        std::shared_ptr<uint8_t[]> frameData = cameraManager_->GetFrameDataArray();
        if (!frameData)
        {
            msleep(10);
            continue;
        }

        ScaleCameraFrameToDisplay(frameData.get(), displayFrame, sourceWidth, sourceHeight);

        textRenderer_.DrawString(0, 16, "Selam canim!", Color(255, 255, 255, 127));
        textRenderer_.RenderOnto(displayFrame, WIDTH, HEIGHT, X_OFFSET, Y_OFFSET);

        // int xOffset = 128;
        // int yOffset = 128;
        // int squareSide = 32;
        // for(int y = yOffset; y < (yOffset + squareSide); ++y)
        // {
        //     for(int x = xOffset * 3; x < (xOffset + squareSide) * 3; x += 3)
        //     {
        //         displayFrame[y * WIDTH * 3 + x] = 255;
        //         displayFrame[y * WIDTH * 3 + x + 1] = 0;
        //         displayFrame[y * WIDTH * 3 + x + 2] = 0;
        //     }
        // }

        RGB888ToRGB565(displayFrame, displayFrameRGB565);
        
        lcd.Show(displayFrameRGB565);
    }
}