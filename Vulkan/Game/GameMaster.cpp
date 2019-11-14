 #include "GameMaster.h"
#include "../System.h"
#include "../Graphics/GraphicsMaster.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Graphics/ShaderParams.h"
#include "../Assets/Terrain.h"
#include "../Assets/Skysphere.h"
#include "../Graphics/Material.h"
#include "../Graphics/TextureManager.h"
#include "../Assets/AltAtmosphere.h"
#include "Camera.h"
#include "SunScript.h"
#include "AtmosphereScript.h"
#include "Scene.h"
#include "FireSystem.h"
#include "../Graphics/GraphicsTypes.h"
#include "../Graphics/SceneDescriptorInfo.h"

#include "../Graphics/StorageBuffer.h"
#include "../Graphics/Descriptor.h"

using namespace QZL;
using namespace Game;

GameMaster::GameMaster(const SystemMasters& masters) 
	: masters_(masters), activeSceneIdx_(0)
{
	scenes_.push_back(new Scene()); // Default scene
}

GameMaster::~GameMaster()
{
	for (auto i = 0; i < scenes_.size(); ++i) {
		SAFE_DELETE(scenes_[i]);
	}
}

void GameMaster::loadDescriptors()
{/*
	Graphics::SceneDescriptorInfo info;
	std::unordered_map<Graphics::RendererTypes, uint32_t> instancesMap;
	scenes_[activeSceneIdx_]->findDescriptorRequirements(instancesMap);
	size_t totalMVPSize = 0;
	size_t totalParamsSize = 0;
	for (auto& it : instancesMap) {
		info.mvpOffsets[(size_t)it.first] = totalMVPSize;
		info.paramsOffsets[(size_t)it.first] = totalParamsSize;
		totalMVPSize += sizeof(glm::mat4) * it.second;
		totalParamsSize += Graphics::Material::materialSizeLUT[(size_t)it.first] * it.second;
	}
	const size_t frameImageSize = 3;
	info.paramsBuffer = Graphics::DescriptorBuffer::makeBuffer<Graphics::StorageBuffer>(masters_.getLogicDevice(), Graphics::MemoryAllocationPattern::kDynamicResource, 0, 0,
		totalParamsSize * frameImageSize, VK_SHADER_STAGE_ALL_GRAPHICS, "ParamsBuffer");
	info.mvpBuffer = Graphics::DescriptorBuffer::makeBuffer<Graphics::StorageBuffer>(masters_.getLogicDevice(), Graphics::MemoryAllocationPattern::kDynamicResource, 1, 0,
		totalMVPSize * frameImageSize, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "MVPBuffer");

	info.instanceDataLayout = masters_.getLogicDevice()->getPrimaryDescriptor()->makeLayout({ info.paramsBuffer->getBinding(), info.mvpBuffer->getBinding() });*/
}

void GameMaster::loadGame()
{
	GameScriptInitialiser scriptInit;
	scriptInit.inputManager = masters_.inputManager;
	scriptInit.system = masters_.system;

	Entity* camera = new Entity("mainCamera");
	scriptInit.owner = camera;
	camera->setGameScript(new Camera(masters_));

	Entity* teapot = new Entity("Teapot");
	teapot->getTransform()->scale = glm::vec3(2.0f);
	teapot->setGraphicsComponent(Graphics::RendererTypes::kStatic, nullptr, new Graphics::StaticShaderParams(),
		masters_.textureManager->requestMaterial(Graphics::MaterialType::kStatic, "ExampleStatic"), "Teapot");
	
	Entity* terrain = new Terrain("terrain", masters_.textureManager);

	Entity* sun = new Entity("sun");
	scriptInit.owner = sun;
	auto sunScript = new SunScript(masters_);
	sun->setGameScript(sunScript);
	auto sunRobject = sunScript->makeRenderObject("SunSystem");
	sun->setGraphicsComponent(Graphics::RendererTypes::kParticle, sunRobject);

	Skysphere* skysphere = new Skysphere("sky", masters_.getLogicDevice(), sunScript, scriptInit);

	Entity* fire = new Entity("firetest");
	scriptInit.owner = fire;
	auto fireScript = new FireSystem(masters_);
	fire->setGameScript(fireScript);
	auto fireRobject = fireScript->makeRenderObject("FireSystem");
	fire->setGraphicsComponent(Graphics::RendererTypes::kParticle, fireRobject);

	auto cameraNode = scenes_[activeSceneIdx_]->addEntity(camera);
	scenes_[activeSceneIdx_]->addEntity(sun, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(skysphere, camera, cameraNode);
	scenes_[activeSceneIdx_]->addEntity(fire);
	scenes_[activeSceneIdx_]->addEntity(terrain);
	scenes_[activeSceneIdx_]->addEntity(teapot);
	scenes_[activeSceneIdx_]->start();

	DEBUG_LOG(scenes_[activeSceneIdx_]);

	masters_.graphicsMaster->registerComponent(terrain->getGraphicsComponent());
	masters_.graphicsMaster->registerComponent(sun->getGraphicsComponent(), sunRobject);
	masters_.graphicsMaster->registerComponent(skysphere->getGraphicsComponent());
	masters_.graphicsMaster->registerComponent(skysphere->getGraphicsComponent(), nullptr, Graphics::RendererTypes::kAtmospherePostProcess);
	masters_.graphicsMaster->registerComponent(fire->getGraphicsComponent(), fireRobject);
	masters_.graphicsMaster->registerComponent(teapot->getGraphicsComponent());
}

void GameMaster::update(float dt)
{
	scenes_[activeSceneIdx_]->update(dt);
}
