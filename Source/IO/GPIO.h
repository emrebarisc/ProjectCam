#ifndef __GPIO_H__
#define __GPIO_H__

class GPIO
{
public:
    explicit GPIO(bool use_bl);
    ~GPIO();

    GPIO(const GPIO&) = delete;
    GPIO& operator=(const GPIO&) = delete;

    void DC(bool high);
    void Reset(bool high);
    void bl(bool high);

private:
    int lineFd_ = -1;
    bool useBl_;

    void Set(int index, bool high);
};

#endif