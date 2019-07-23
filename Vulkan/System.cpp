#include "System.h"
#include "Graphics/GraphicsMaster.h"
#include "Game/GameMaster.h"
#include "Assets/AssetManager.h"
#include "Graphics/TextureLoader.h"
#include "Graphics/LogicDevice.h"
#include "InputManager.h"

using namespace QZL;

int main(int argc, char** argv) {
	System system;
	system.loop();
	return 0;
}

System::System()
{
	// Asset manager must be created before graphics master
	masters_.assetManager = new Assets::AssetManager();

	masters_.graphicsMaster = new Graphics::GraphicsMaster(masters_);
	inputManager_ = new InputManager(masters_.graphicsMaster->details_.window);
	masters_.physicsMaster = nullptr;

	// Graphics master must be created before texture loader for logic device, and texture loader must be created before game master
	masters_.assetManager->textureLoader = new Graphics::TextureLoader(masters_.graphicsMaster->details_.logicDevice);

	masters_.gameMaster = new Game::GameMaster(masters_);
	masters_.gameMaster->loadGame();
}

System::~System()
{
	SAFE_DELETE(masters_.assetManager);
	SAFE_DELETE(masters_.gameMaster);
	SAFE_DELETE(masters_.physicsMaster);
	SAFE_DELETE(masters_.graphicsMaster);
}

void System::loop()
{
	masters_.graphicsMaster->preframeSetup();
	Shared::PerfMeasurer perfMeasurer;
	while (!glfwWindowShouldClose(masters_.graphicsMaster->details_.window)) {
		glfwPollEvents();
		perfMeasurer.startTime();
		masters_.graphicsMaster->loop();

		perfMeasurer.endTime();
	}
	std::cout << "Vulkan perf: " << perfMeasurer.getAverageTime().count() << std::endl;
	vkDeviceWaitIdle(*masters_.graphicsMaster->details_.logicDevice);
}
