#pragma once

#include <memory>

class CameraManager;
class Display;
class InputManager;

class Program
{
	public:
		~Program();

		static Program* GetInstance()
		{
			if(instance_ == nullptr)
			{
				instance_ = new Program();
			}

			return instance_;
		}

		void Init();
		void Start();

		std::shared_ptr<CameraManager> GetCameraManager() const
		{
			return cameraManager_;
		}

		std::shared_ptr<Display> GetDisplay() const
		{
			return Display_;
		}

		std::shared_ptr<InputManager> GetInputManager() const
		{
			return inputManager_;
		}

	protected:

	private:
		Program();
		Program(Program& rhs) = delete;
		Program operator=(Program& rhs) = delete;

		static Program* instance_;

		std::shared_ptr<CameraManager> cameraManager_{ nullptr };
		std::shared_ptr<Display> Display_{ nullptr };
		std::shared_ptr<InputManager> inputManager_{ nullptr };

		unsigned char pendingClose_ : 1;
};
