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
	// Trees
	const std::vector<glm::vec3> positions = {
		{180, 165, 72}, {153, 184, 96}, { 132, 168, 39 }, {126, 161, 15},
		{84, 157, 6}, {65,135,14},{27, 117, 3}, {6, 116, 37}, {38, 118, 37}, {50,120,92},
		{30, 118, 116}, {13,113,62},{330,90,505}, {192, 135, 15}, {245, 147, 67}, {271,114,27},
		{402, 90, 57}, {425,95,107}, {426,86,73}, {458,77,64},{426, 63, 3}, { 447, 92, 122 },
		{ 504, 66, 85 }, {481, 87, 123},{ 467, 96, 166 }, { 489,90,159 }, { 505,87,153 }, { 552,79,165 },
		{ 520,93,212 }, { 482,98,202 }, { 490,98,224 }, { 498,99,266 }, {446, 84, 86},{458,82,106},
		{441,89,108}, {420, 95, 99}, {478, 84, 140}
	};
	for (auto pos : positions) {
		Entity* tree = new Entity("lowpoly_tree");
		tree->getTransform()->setScale(3.0f);
		tree->getTransform()->position = pos;
		tree->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(),
			masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "ExampleStatic"), "tree");
		scene->addEntity(tree);
	}

	// Windmill
	const std::vector<glm::vec3> millPositions = {
		{759, 96, 413}
	};
	for (auto pos : millPositions) {
		Entity* turbineBase = new Entity("turbine_base");
		turbineBase->getTransform()->setScale(20.0f);
		turbineBase->getTransform()->rotationAxis = glm::vec3(0.0, 1.0, 0.0);
		turbineBase->getTransform()->rotationAngle = glm::radians(180.0f);
		turbineBase->getTransform()->position = pos;
		turbineBase->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(20.0f),
			masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "turbine"), "TurbineBase");
		auto baseNode = scene->addEntity(turbineBase);
		Entity* turbineBlade = new Entity("turbine_blade");
		turbineBlade->getTransform()->position.y = 3.43f;
		turbineBlade->getTransform()->rotationAxis = glm::vec3(1.0, 0.0, 0.0);
		turbineBlade->setSimpleUpdateFunction([turbineBlade](float dt) { 
			turbineBlade->getTransform()->rotationAngle += dt; 
			if (turbineBlade->getTransform()->rotationAngle > glm::two_pi<float>()) {
				turbineBlade->getTransform()->rotationAngle = 0.0f;
			}
		});
		turbineBlade->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(10.0f),
			masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "turbine_blade"), "TurbineBladesAdjusted");
		scene->addEntity(turbineBlade, turbineBase, baseNode);
	}

	// Lamps
	const std::vector<glm::vec3> lampPositions = {
		{802, 102, 399}, {810,102,378},{823,97,365}, {811, 90, 346}, {833, 90, 337},
		{755,101,365}, {745,87,318}, {698, 98, 340}, {717, 89, 317}, {708, 87, 303}
	};
	for (auto pos : lampPositions) {
		Entity* lampPost = new Entity("lamp_post");
		lampPost->getTransform()->setScale(0.05f);
		lampPost->getTransform()->position = pos;
		lampPost->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(),
			masters_.textureManager->requestMaterial(Graphics::RendererTypes::kStatic, "posts"), "poste_obj");
		auto baseNode = scene->addEntity(lampPost);
		auto radius = rand() % 200 + 20;
		Entity* light = new LightSource("lampLight", glm::normalize(pos * glm::vec3((float)(rand() % 100), (float)(rand() % 100), (float)(rand() % 100))), 
			radius, 1500.0f, &lampPost->getTransform()->position);
		light->getTransform()->position = pos;
		light->getTransform()->setScale(1500.0f);
		scene->addEntity(light);
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

	Entity* light = new LightSource("light", glm::vec3(0.5), 2000.0f, 1500.0f, &masters_.graphicsMaster->getCamera(1)->position);
	light->getTransform()->setScale(1500.0f);

	loadStatics(scenes_[activeSceneIdx_]);

	auto cameraNode = scenes_[activeSceneIdx_]->addEntity(camera);
	scenes_[activeSceneIdx_]->addEntity(skysphere, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(water);
	scenes_[activeSceneIdx_]->addEntity(terrain);
	scenes_[activeSceneIdx_]->addEntity(sun);
	//scenes_[activeSceneIdx_]->addEntity(teapotDeferred);
	//scenes_[activeSceneIdx_]->addEntity(teapotDeferred2);
	scenes_[activeSceneIdx_]->addEntity(light);

	DEBUG_LOG(scenes_[activeSceneIdx_]);
}

void GameMaster::start()
{
	scenes_[activeSceneIdx_]->start();
}
