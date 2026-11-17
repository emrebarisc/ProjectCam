#include "CameraManager.h"

#include <chrono>
#include <thread>
#include <cstring>
#include <iostream>

#include <sys/mman.h>

#include "libcamera/libcamera.h"
#include "Program.h"

using namespace std::chrono_literals;

CameraManager::CameraManager()
{
	isCameraRunning_ = false;
	colorDepth_ = (int)pixelFormat_ % 10;

	switch(pixelFormat_)
	{
		case PixelFormat::R8:
			libcameraPixelFormat_ = libcamera::formats::XRGB8888;
			break;
		case PixelFormat::YUV420:
			libcameraPixelFormat_ = libcamera::formats::YUV420;
			break;
		case PixelFormat::RGB565:
			libcameraPixelFormat_ = libcamera::formats::RGB565;
			break;
		case PixelFormat::RGB888:
			libcameraPixelFormat_ = libcamera::formats::RGB888;
			break;
		case PixelFormat::XRGB8888:
			libcameraPixelFormat_ = libcamera::formats::XRGB8888;
			break;
		case PixelFormat::RGBA8888:
			libcameraPixelFormat_ = libcamera::formats::RGBA8888;
			break;
		default:
			break;
	}
}

CameraManager::~CameraManager()
{
	if (libcameraCamera_)
	{
		libcameraCamera_->stop();
	}
	if (frameBufferAllocator_ && libcameraCameraStream_)
	{
		frameBufferAllocator_->free(libcameraCameraStream_);
	}
	delete frameBufferAllocator_;
	frameBufferAllocator_ = nullptr;

	if (libcameraCamera_)
	{
		libcameraCamera_->release();
		libcameraCamera_.reset();
	}
	if (libcameraCameraManager_)
	{
		libcameraCameraManager_->stop();
	}
}

void CameraManager::RequestComplete(libcamera::Request* request)
{
	if(request->status() == libcamera::Request::RequestCancelled)
	{
		return;
	}

	const std::map<const libcamera::Stream*, libcamera::FrameBuffer*>& frameBuffers = request->buffers();

	for(auto frameBufferPair : frameBuffers)
	{
		std::lock_guard<std::mutex> frameLock(lastlyCapturedFrameMutex_);
		lastlyCapturedFrame_ = frameBufferPair.second;
	}

	request->reuse(libcamera::Request::ReuseBuffers);
	libcameraCamera_->queueRequest(request);
}

void CameraManager::Init()
{
	libcameraCameraManager_ = std::make_unique<libcamera::CameraManager>();
	libcameraCameraManager_->start();

	auto cameras = libcameraCameraManager_->cameras();
	if(cameras.empty())
	{
		std::cout << "No camera found!" << std::endl;
		return;
	}
	
	std::string cameraId = cameras[0]->id();

	libcameraCamera_ = libcameraCameraManager_->get(cameraId);
	if (libcameraCamera_)
	{
		libcameraCamera_->acquire();
	}

	SetupHighQualityCamera();
}

void CameraManager::Start()
{
	if (!libcameraCamera_ || !frameBufferAllocator_ || !libcameraCameraStream_)
	{
		std::cerr << "Camera not properly initialized before Start()" << std::endl;
		return;
	}

	const std::vector<std::unique_ptr<libcamera::FrameBuffer>>& frameBuffers = frameBufferAllocator_->buffers(libcameraCameraStream_);
	
	for(unsigned int i = 0; i < frameBuffers.size(); ++i)
	{
		std::unique_ptr<libcamera::Request> request = libcameraCamera_->createRequest();
		if(!request)
		{
			std::cerr << "Can't create request" << std::endl;
			return;
		}

		const std::unique_ptr<libcamera::FrameBuffer>& frameBuffer = frameBuffers[i];
		int ret = request->addBuffer(libcameraCameraStream_, frameBuffer.get());
		if(ret < 0)
		{
			std::cerr << "Can't set buffer for request" << std::endl;
			return;
		}

		libcameraRequests_.push_back(std::move(request));
	}

	libcameraCamera_->requestCompleted.connect(this, &CameraManager::RequestComplete);
	
	std::this_thread::sleep_for(3000ms);
	
	libcameraCamera_->start();
	for(std::unique_ptr<libcamera::Request>& request : libcameraRequests_)
	{
		libcameraCamera_->queueRequest(request.get());
	}
	isCameraRunning_ = true;
}

void CameraManager::SetupHighQualityCamera()
{
	cameraWidth_ = HIGH_QUALITY_RESOLUTION_WIDTH;
	cameraHeight_ = HIGH_QUALITY_RESOLUTION_HEIGHT;
	colorDepth_ = 1;
	
	libcameraPixelFormat_ = libcamera::formats::XRGB8888; 

	SetupCamera();
}

void CameraManager::SetupCamera()
{
	if (!libcameraCamera_) return;

	std::unique_ptr<libcamera::CameraConfiguration> cameraConfig = libcameraCamera_->generateConfiguration({ libcamera::StreamRole::Viewfinder });
	if (!cameraConfig) return;

	libcamera::StreamConfiguration& streamConfig = cameraConfig->at(0);
	std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;

	streamConfig.size.width = cameraWidth_;
	streamConfig.size.height = cameraHeight_;
	streamConfig.pixelFormat = libcameraPixelFormat_;

	cameraConfig->validate();

	std::cout << "Validated viewfinder configuration is: " << streamConfig.toString() << std::endl;

	libcameraCamera_->configure(cameraConfig.get());

	frameBufferAllocator_ = new libcamera::FrameBufferAllocator(libcameraCamera_);

	for(libcamera::StreamConfiguration& streamConfiguration : *cameraConfig)
	{
		int ret = frameBufferAllocator_->allocate(streamConfiguration.stream());
		if(ret < 0)
		{
			std::cerr << "Can't allocate buffers" << std::endl;
			return;
		}

		size_t allocatedSize = frameBufferAllocator_->buffers(streamConfiguration.stream()).size();
		std::cout << "Allocated " << allocatedSize << " buffers for stream" << std::endl;
	}

	libcameraCameraStream_ = streamConfig.stream();
}

std::shared_ptr<uint8_t[]> CameraManager::GetFrameDataArray()
{
	std::lock_guard<std::mutex> frameLock(lastlyCapturedFrameMutex_);

	if(!lastlyCapturedFrame_)
	{
		return nullptr;
	}

	if (lastlyCapturedFrame_->planes().empty())
	{
		std::cerr << "No planes in buffer!" << std::endl;
		return nullptr;
	}

	int fd = lastlyCapturedFrame_->planes()[0].fd.get();
	size_t length = lastlyCapturedFrame_->planes()[0].length;
	void *data = mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, 0);

	if (data == MAP_FAILED)
	{
		std::cerr << "Failed to map buffer memory!" << std::endl;
		return nullptr;
	}
	
	const int resolution = cameraWidth_ * cameraHeight_;
	// Fixed: Allocated resolution * 3 bytes (RGB888) instead of resolution * 4
	std::shared_ptr<uint8_t[]> frameDataArray(new uint8_t[resolution * 3]);

	uint32_t* pixelData = reinterpret_cast<uint32_t*>(data);

	for(int pixelIndex = 0; pixelIndex < resolution; ++pixelIndex)
	{
		uint32_t pixel = pixelData[pixelIndex];
		frameDataArray[pixelIndex * 3]     = (pixel >> 16) & 0xFF;
		frameDataArray[pixelIndex * 3 + 1] = (pixel >> 8) & 0xFF;
		frameDataArray[pixelIndex * 3 + 2] = pixel & 0xFF;
	}

	munmap(data, length);
	
	return frameDataArray;
}

std::shared_ptr<std::vector<uint8_t>> CameraManager::GetCompressedFrameData()
{
	std::lock_guard<std::mutex> frameLock(lastlyCapturedFrameMutex_);

	if(!lastlyCapturedFrame_ || lastlyCapturedFrame_->planes().empty())
	{
		return nullptr;
	}

	int fd = lastlyCapturedFrame_->planes()[0].fd.get();
	size_t maxLength = lastlyCapturedFrame_->planes()[0].length;
	size_t actualSize = lastlyCapturedFrame_->metadata().planes()[0].bytesused;
	
	if (actualSize == 0) return nullptr;

	void *data = mmap(nullptr, maxLength, PROT_READ, MAP_SHARED, fd, 0);

	if (data == MAP_FAILED)
	{
		std::cerr << "Failed to map buffer memory!" << std::endl;
		return nullptr;
	}
	
	auto frameData = std::make_shared<std::vector<uint8_t>>(actualSize);
	std::memcpy(frameData->data(), data, actualSize);

	munmap(data, maxLength);
	
	return frameData;
}