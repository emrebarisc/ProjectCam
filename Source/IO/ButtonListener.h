


#pragma once

#include <gpiod.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

class ButtonListener 
{
public:
	using Callback = std::function<void()>;
	using Clock = std::chrono::steady_clock;
	using Ms = std::chrono::milliseconds;

	struct Config 
    {
		Ms debounce{ 30 };
		Ms doubleClick{ 300 };
		Ms hold{ 700 };
		Ms pollInterval{ 5 };
	};

private:
	struct RequestDeleter 
    {
		void operator()(gpiod_line_request* request) const 
        {
			if (request)
				gpiod_line_request_release(request);
		}
	};

	using RequestPtr =
		std::unique_ptr<gpiod_line_request, RequestDeleter>;

	enum class Event 
    {
		Single,
		Double,
		Hold
	};

public:
	// Constructor using default configuration.
	ButtonListener(
		unsigned int gpio,
		Callback onSingleClick = nullptr,
		Callback onDoubleClick = nullptr,
		Callback onHold = nullptr)
		: ButtonListener(
			gpio,
			std::move(onSingleClick),
			std::move(onDoubleClick),
			std::move(onHold),
			Config{})
	
            {
	}

	// Constructor using custom configuration.
	ButtonListener(
		unsigned int gpio,
		Callback onSingleClick,
		Callback onDoubleClick,
		Callback onHold,
		Config config)
		: gpio_(gpio),
		single_(std::move(onSingleClick)),
		double_(std::move(onDoubleClick)),
		hold_(std::move(onHold)),
		config_(config)
	
        {
		if (config_.pollInterval.count() <= 0 ||
			config_.debounce.count() < 0 ||
			config_.doubleClick.count() < 0 ||
			config_.hold.count() < 0) 
            {
			throw std::invalid_argument(
				"Invalid button timing configuration");
		}

		using ChipPtr =
			std::unique_ptr<
			gpiod_chip,
			decltype(&gpiod_chip_close)>;

		using SettingsPtr =
			std::unique_ptr<
			gpiod_line_settings,
			decltype(&gpiod_line_settings_free)>;

		using LineConfigPtr =
			std::unique_ptr<
			gpiod_line_config,
			decltype(&gpiod_line_config_free)>;

		using RequestConfigPtr =
			std::unique_ptr<
			gpiod_request_config,
			decltype(&gpiod_request_config_free)>;

		// Open Raspberry Pi GPIO controller.
		ChipPtr chip(
			gpiod_chip_open("/dev/gpiochip0"),
			&gpiod_chip_close);

		if (!chip) 
        {
			throw std::system_error(
				errno,
				std::generic_category(),
				"Cannot open /dev/gpiochip0");
		}

		// Create GPIO settings.
		SettingsPtr settings(
			gpiod_line_settings_new(),
			&gpiod_line_settings_free);

		if (!settings)
			throw std::runtime_error(
				"Cannot create GPIO settings");

		// Configure GPIO as input.
		if (gpiod_line_settings_set_direction(
			settings.get(),
			GPIOD_LINE_DIRECTION_INPUT) < 0)
		
            {
			throw std::system_error(
				errno,
				std::generic_category(),
				"Cannot configure GPIO input");
		}

		// Enable internal pull-up resistor.
		if (gpiod_line_settings_set_bias(
			settings.get(),
			GPIOD_LINE_BIAS_PULL_UP) < 0)
		
            {
			throw std::system_error(
				errno,
				std::generic_category(),
				"Cannot configure GPIO pull-up");
		}

		// Enable internal pull-up resistor.
		if (gpiod_line_settings_set_bias(
			settings.get(),
			GPIOD_LINE_BIAS_PULL_UP) < 0) 
        {
			throw std::system_error(
				errno,
				std::generic_category(),
				"Cannot configure GPIO pull-up");
		}

		// Create line configuration.
		LineConfigPtr lineConfig(
			gpiod_line_config_new(),
			&gpiod_line_config_free);

		if (!lineConfig)
			throw std::runtime_error(
				"Cannot create GPIO line configuration");

		if (gpiod_line_config_add_line_settings(
			lineConfig.get(),
			&gpio_,
			1,
			settings.get()) < 0) 
        {
			throw std::system_error(
				errno,
				std::generic_category(),
				"Cannot add GPIO line settings");
		}

		// Create request configuration.
		RequestConfigPtr requestConfig(
			gpiod_request_config_new(),
			&gpiod_request_config_free);

		if (!requestConfig)
			throw std::runtime_error(
				"Cannot create GPIO request configuration");

		gpiod_request_config_set_consumer(
			requestConfig.get(),
			"ProjectCam ButtonListener");

		// Request exclusive ownership of GPIO line.
		request_.reset(
			gpiod_chip_request_lines(
				chip.get(),
				requestConfig.get(),
				lineConfig.get()));

		if (!request_) 
        {
			throw std::system_error(
				errno,
				std::generic_category(),
				"Cannot request GPIO " +
				std::to_string(gpio_));
		}

		worker_ = std::thread(&ButtonListener::Run, this);
	}

	~ButtonListener() 
    {
		Stop();
	}

	ButtonListener(const ButtonListener&) = delete;
	ButtonListener& operator=(const ButtonListener&) = delete;

	void Stop() 
    {
		running_ = false;

		if (worker_.joinable())
		{
            worker_.join();
        }

		request_.reset();
	}

	void SetSingleClickCallback(Callback callback) 
    {
		std::lock_guard<std::mutex> lock(callbackMutex_);
		single_ = std::move(callback);
	}

	void SetDoubleClickCallback(Callback callback) 
    {
		std::lock_guard<std::mutex> lock(callbackMutex_);
		double_ = std::move(callback);
	}

	void SetHoldCallback(Callback callback) 
    {
		std::lock_guard<std::mutex> lock(callbackMutex_);
		hold_ = std::move(callback);
	}

	void ClearSingleClickCallback() 
    {
		SetSingleClickCallback(nullptr);
	}

	void ClearDoubleClickCallback() 
    {
		SetDoubleClickCallback(nullptr);
	}

	void ClearHoldCallback() 
    {
		SetHoldCallback(nullptr);
	}

private:
	unsigned int gpio_;

	Callback single_;
	Callback double_;
	Callback hold_;

	Config config_;

	RequestPtr request_;

	std::atomic<bool> running_{ true };

	std::thread worker_;

	std::mutex callbackMutex_;

	// Read GPIO value.
	// Active-low: 0 = pressed, 1 = released.
	int ReadValue() 
    {
		const auto value =
			gpiod_line_request_get_value(
				request_.get(),
				gpio_);

		if (value == GPIOD_LINE_VALUE_ERROR)
			return -1;

		return value == GPIOD_LINE_VALUE_ACTIVE ? 1 : 0;
	}

	void Invoke(Event event) 
    {
		Callback callback;

        {
			std::lock_guard<std::mutex> lock(callbackMutex_);

			switch (event) 
            {
			case Event::Single:
				callback = single_;
				break;

			case Event::Double:
				callback = double_;
				break;

			case Event::Hold:
				callback = hold_;
				break;
			}
		}

		if (callback)
			callback();
	}

	void Run() 
    {
		auto now = Clock::now();

		int raw = ReadValue();

		if (raw < 0) 
        {
			running_ = false;
			return;
		}

		int candidate = raw;
		int stable = raw;

		bool pressed = (stable == 0);
		bool holdFired = false;
		bool pendingSingle = false;

		auto candidateSince = now;
		auto pressStart = now;
		auto clickDeadline = now;

		while (running_) 
        {
			std::this_thread::sleep_for(
				config_.pollInterval);

			now = Clock::now();

			raw = ReadValue();

			if (raw < 0) 
            {
				running_ = false;
				break;
			}

			// Detect potential state change.
			if (raw != candidate) 
            {
				candidate = raw;
				candidateSince = now;
			}

			// Debounce.
			if (candidate != stable &&
				now - candidateSince >= config_.debounce) 
                {

				stable = candidate;

				if (stable == 0) 
                {
					// Button pressed.
					pressed = true;
					holdFired = false;
					pressStart = now;
				}
				else if (pressed) 
                {
					// Button released.
					pressed = false;

					if (!holdFired) 
                    {
						if (pendingSingle && now <= clickDeadline) 
                        {
							pendingSingle = false;
							Invoke(Event::Double);
						}
						else 
                        {
							if (pendingSingle)
                            {
                                Invoke(Event::Single);
                            }

							pendingSingle = true;

							clickDeadline = now + config_.doubleClick;
						}
					}
				}
			}

			// Long press detection.
			if (pressed && !holdFired && config_.hold <= (now - pressStart)) 
            {
				holdFired = true;
				pendingSingle = false;

				Invoke(Event::Hold);
			}

			// Single click timeout.
			if (pendingSingle && clickDeadline <= now) 
            {
				pendingSingle = false;
				Invoke(Event::Single);
			}
		}
	}
};
