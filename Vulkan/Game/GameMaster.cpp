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
#include "../Assets/LightSource.h"
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

void GameMaster::loadStatics(Scene* scene)
{
	const std::vector<glm::vec3> positions = {
		{180, 169, 72}, {153, 186, 96}, { 132, 170, 39 }, {126, 164, 15},
		{84, 159, 6}, {65,137,14},{27, 120, 3}, {6, 119, 37}, {38, 121, 37}, {50,122,92},
		{30, 121, 116}, {13,115,62},{330,93,505}
	};
	for (auto pos : positions) {
		Entity* tree = new Entity("lowpoly_tree");
		tree->getTransform()->setScale(3.0f);
		tree->getTransform()->position = pos;
		tree->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(),
			masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "ExampleStatic"), "tree");
		scene->addEntity(tree);
	}
}

void GameMaster::loadGame()
{
	GameScriptInitialiser scriptInit;
	scriptInit.inputManager = masters_.inputManager;
	scriptInit.system = masters_.system;

	Entity* camera = new Entity("mainCamera");
	scriptInit.owner = camera;
	camera->setGameScript(new Camera(masters_));

	Entity* terrain = new Terrain("terrain", masters_.textureManager);
	terrain->setGameScript(new TerrainScript(masters_));

	Entity* sun = new Entity("sun");
	scriptInit.owner = sun;
	auto sunScript = new SunScript(masters_);
	sun->setGameScript(sunScript);
	//sun->setGraphicsComponent(Graphics::RendererTypes::kParticle, sunScript->makeShaderParams(), "sun", sunScript->getMaterial());

	Skysphere* skysphere = new Skysphere("sky", masters_.getLogicDevice(), sunScript, scriptInit);

	Entity* water = new Water("water", masters_.textureManager);
	water->getTransform()->position = glm::vec3(0.0f, 98.0f, 0.0f);

	/*Entity* teapotDeferred = new Entity("TeapotDeferred");
	teapotDeferred->getTransform()->position = glm::vec3(200.0f, 50.0f, 200.0f);
	teapotDeferred->getTransform()->setScale(2.0f);
	teapotDeferred->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(),
		masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "ExampleStatic"), "Teapot");*/

	/*Entity* teapotDeferred2 = new Entity("TeapotDeferred2");
	teapotDeferred2->getTransform()->position = glm::vec3(400.0f, 50.0f, 300.0f);
	teapotDeferred2->getTransform()->setScale(2.0f);
	teapotDeferred2->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(),
		masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "ExampleStatic"), "Teapot");*/

	Entity* light = new LightSource("light", glm::vec3(0.8), 2000.0f, 1.0f);
	light->getTransform()->setScale(1500.0f);

	loadStatics(scenes_[activeSceneIdx_]);

	auto cameraNode = scenes_[activeSceneIdx_]->addEntity(camera);
	scenes_[activeSceneIdx_]->addEntity(skysphere, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(water);
	scenes_[activeSceneIdx_]->addEntity(terrain);
	scenes_[activeSceneIdx_]->addEntity(sun);
	//scenes_[activeSceneIdx_]->addEntity(teapotDeferred);
	//scenes_[activeSceneIdx_]->addEntity(teapotDeferred2);
	scenes_[activeSceneIdx_]->addEntity(light, sun);

	DEBUG_LOG(scenes_[activeSceneIdx_]);
}

void GameMaster::start()
{
	scenes_[activeSceneIdx_]->start();
}
