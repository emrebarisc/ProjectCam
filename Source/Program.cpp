#include "Program.h"

#include "Managers/CameraManager.h"
#include "IO/Display.h"
#include "Managers/InputManager.h"

Program* Program::instance_ = nullptr;

Program::Program()
{
	if(instance_ == nullptr)
	{
		instance_ = this;
	}

	cameraManager_ = std::make_shared<CameraManager>();
	Display_ = std::make_shared<Display>(cameraManager_);
	inputManager_ = std::make_shared<InputManager>();

	pendingClose_ = false;
}

Program::~Program()
{
}

void Program::Init()
{
	cameraManager_->Init();
	Display_->Init();
	inputManager_->Init();
}

void Program::Start()
{
	cameraManager_->Start();
	Display_->Start();

	while(!pendingClose_);
}
