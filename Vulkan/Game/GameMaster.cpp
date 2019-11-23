 #include "GameMaster.h"
#include "../System.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Graphics/ShaderParams.h"
#include "../Assets/Terrain.h"
#include "../Assets/Skysphere.h"
#include "../Assets/Water.h"
#include "../Graphics/Material.h"
#include "../Graphics/TextureManager.h"
#include "../Assets/AltAtmosphere.h"
#include "Camera.h"
#include "SunScript.h"
#include "AtmosphereScript.h"
#include "Scene.h"
#include "FireSystem.h"
#include "RainSystem.h"
#include "TerrainScript.h"
#include "../Graphics/GraphicsTypes.h"
#include "../Graphics/SceneDescriptorInfo.h"

#include "../Graphics/StorageBuffer.h"
#include "../Graphics/Descriptor.h"
#include "../Graphics/PhysicalDevice.h"
#include "../Graphics/LogicDevice.h"
#include "../Graphics/RenderObject.h"

using namespace QZL;
using namespace Game;

GameMaster::GameMaster(const SystemMasters& masters) 
	: masters_(masters), activeSceneIdx_(0)
{
	scenes_.push_back(new Scene(&masters)); // Default scene
}

GameMaster::~GameMaster()
{
	for (auto i = 0; i < scenes_.size(); ++i) {
		SAFE_DELETE(scenes_[i]);
	}
}

Graphics::SceneGraphicsInfo* GameMaster::loadDescriptors()
{
	return scenes_[activeSceneIdx_]->createDescriptors(3, masters_.graphicsMaster->details_.physicalDevice->getDeviceLimits());
}

void GameMaster::loadGame()
{
	GameScriptInitialiser scriptInit;
	scriptInit.inputManager = masters_.inputManager;
	scriptInit.system = masters_.system;

	Entity* camera = new Entity("mainCamera");
	scriptInit.owner = camera;
	camera->setGameScript(new Camera(masters_));

	/*Entity* teapot = new Entity("Teapot");
	teapot->getTransform()->position = glm::vec3(100.0f, 40.0f, 300.0f);
	teapot->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(),
		masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "ExampleStatic"), "Teapot");
	
	Entity* terrain = new Terrain("terrain", masters_.textureManager);
	terrain->setGameScript(new TerrainScript(masters_));

	Entity* water = new Water("water", masters_.textureManager);
	water->getTransform()->position = glm::vec3(0.0f, 80.0f, 0.0f);

	Entity* rain = new Entity("rain");
	scriptInit.owner = rain;
	auto rainScript = new RainSystem(masters_);
	rain->setGameScript(rainScript);
	rain->setGraphicsComponent(Graphics::RendererTypes::kParticle, rainScript->makeShaderParams(), "rain", rainScript->getMaterial());*/

	Entity* sun = new Entity("sun");
	scriptInit.owner = sun;
	auto sunScript = new SunScript(masters_);
	sun->setGameScript(sunScript);
	//sun->setGraphicsComponent(Graphics::RendererTypes::kParticle, sunScript->makeShaderParams(), "sun", sunScript->getMaterial());

	Skysphere* skysphere = new Skysphere("sky", masters_.getLogicDevice(), sunScript, scriptInit);

	Entity* teapotDeferred = new Entity("TeapotDeferred");
	teapotDeferred->getTransform()->position = glm::vec3(200.0f, 0.0f, 200.0f);
	teapotDeferred->getTransform()->setScale(2.0f);
	teapotDeferred->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(),
		masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "ExampleStatic"), "Teapot");

	Entity* light = new Entity("light");
	light->setGraphicsComponent(Graphics::RendererTypes::kLight, nullptr, "ico", nullptr);
	light->getTransform()->position = glm::vec3(200.0f, 0.0f, 200.0f);
	light->getTransform()->setScale(100.0f);

	auto cameraNode = scenes_[activeSceneIdx_]->addEntity(camera);
	scenes_[activeSceneIdx_]->addEntity(skysphere, camera, cameraNode);
	//scenes_[activeSceneIdx_]->addEntity(rain);
	//scenes_[activeSceneIdx_]->addEntity(water);
	//scenes_[activeSceneIdx_]->addEntity(terrain);
	//scenes_[activeSceneIdx_]->addEntity(sun, terrain);
	scenes_[activeSceneIdx_]->addEntity(teapotDeferred);
	scenes_[activeSceneIdx_]->addEntity(light);

	DEBUG_LOG(scenes_[activeSceneIdx_]);
}

void GameMaster::start()
{
	scenes_[activeSceneIdx_]->start();
}
