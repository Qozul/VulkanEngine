#include "System.h"
#include "Graphics/GraphicsMaster.h"
#include "Game/GameMaster.h"
#include "Graphics/TextureLoader.h"
#include "Graphics/LogicDevice.h"
#include "InputManager.h"
#include "Graphics/OptionalExtensions.h"
#include "Graphics/TextureManager.h"
#include <chrono>

using namespace QZL;
using namespace QZL::Graphics;
using Clock = std::chrono::high_resolution_clock;

int main(int argc, char** argv) {
	System system;
	system.loop();
	return 0;
}

float System::deltaTime = 0.0f;
float System::deltaTimeSeconds = 0.0f;

System::System()
{
	masters_.graphicsMaster = new Graphics::GraphicsMaster(masters_);
	inputManager_ = new InputManager(masters_.graphicsMaster->details_.window);
	masters_.system = this;
	masters_.inputManager = inputManager_;
	masters_.physicsMaster = nullptr;
	masters_.gameMaster = new Game::GameMaster(masters_);
	masters_.gameMaster->loadGame();
	Graphics::SceneGraphicsInfo* graphicsInfo = masters_.gameMaster->loadDescriptors();
	masters_.graphicsMaster->initialiseRenderPath(masters_.gameMaster->getActiveScene(), graphicsInfo);
	masters_.gameMaster->start();
}

System::~System()
{
	SAFE_DELETE(masters_.gameMaster);
	SAFE_DELETE(masters_.textureManager);
	SAFE_DELETE(masters_.graphicsMaster);
}

void System::loop()
{
	masters_.graphicsMaster->preframeSetup();
	Shared::PerfMeasurer perfMeasurer;
	Clock::time_point lastTime = Clock::now();
	while (!glfwWindowShouldClose(masters_.graphicsMaster->details_.window) 
		&& !glfwGetKey(masters_.graphicsMaster->details_.window, GLFW_KEY_ESCAPE)) {
		auto measuredTime = Clock::now();
		std::chrono::duration<float, std::milli> diff = (measuredTime - lastTime);
		deltaTime = diff.count();
		deltaTimeSeconds = deltaTime / 1000.0f;
		lastTime = measuredTime;

		glfwPollEvents();
		inputManager_->checkInput(deltaTimeSeconds);
		perfMeasurer.startTime();
		masters_.graphicsMaster->loop();
		perfMeasurer.endTime();
	}
	std::cout << "Vulkan perf: " << perfMeasurer.getAverageTime().count() << std::endl;
	vkDeviceWaitIdle(*masters_.graphicsMaster->details_.logicDevice);
}
