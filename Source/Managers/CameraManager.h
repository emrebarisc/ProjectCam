#pragma once

#include "IManager.h"

#include <memory>
#include <vector>

#include "libcamera/libcamera.h"

constexpr int HIGH_QUALITY_RESOLUTION_WIDTH = 640;
constexpr int HIGH_QUALITY_RESOLUTION_HEIGHT = 480;
constexpr int HIGH_QUALITY_RESOLUTION = HIGH_QUALITY_RESOLUTION_WIDTH * HIGH_QUALITY_RESOLUTION_HEIGHT;

constexpr int DISPLAY_RESOLUTION_WIDTH = 280;
constexpr int DISPLAY_RESOLUTION_HEIGHT = 240;
constexpr int DISPLAY_RESOLUTION = DISPLAY_RESOLUTION_WIDTH * DISPLAY_RESOLUTION_HEIGHT;

enum class PixelFormat : uint8_t
{
	None = 0,
	R8 = 10,
	YUV420 = 30,
	RGB565,
	RGB888,
	XRGB8888 = 40,
	RGBA8888
};

class CameraManager : public IManager
{
public:
	CameraManager();
	~CameraManager();

	void Init() override;
	void Start();

	std::shared_ptr<uint8_t[]> GetFrameDataArray();
	std::shared_ptr<std::vector<uint8_t>> GetCompressedFrameData();

	void SetupHighQualityCamera();

	int GetCameraWidth() const
	{
		return cameraWidth_;
	}

	int GetCameraHeight() const
	{
		return cameraHeight_;
	}

	int GetColorDepth() const
	{
		return colorDepth_;
	}

private:
	void RequestComplete(libcamera::Request* request);
	void SetupCamera();

	std::shared_ptr<libcamera::Camera> libcameraCamera_{ nullptr };
	std::unique_ptr<libcamera::CameraManager> libcameraCameraManager_{ nullptr };
	libcamera::FrameBuffer* lastlyCapturedFrame_{ nullptr };
	libcamera::FrameBufferAllocator* frameBufferAllocator_{ nullptr }; 
	libcamera::Stream* libcameraCameraStream_{ nullptr };
	std::vector<std::unique_ptr<libcamera::Request>> libcameraRequests_{};

	std::mutex lastlyCapturedFrameMutex_;

	unsigned int cameraWidth_{ HIGH_QUALITY_RESOLUTION_WIDTH };
	unsigned int cameraHeight_{ HIGH_QUALITY_RESOLUTION_HEIGHT };
	unsigned int colorDepth_ { 4 };

	PixelFormat pixelFormat_{ PixelFormat::RGB888 };
	libcamera::PixelFormat libcameraPixelFormat_{ libcamera::formats::RGB888 };

	uint8_t isCameraRunning_ : 1;
};