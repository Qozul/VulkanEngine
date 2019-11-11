#include "SceneLoader.h"
#include "Scene.h"
#include "GameScript.h"
#include "../Assets/Entity.h"
#include "../Assets/Transform.h"
#include "../Graphics/GraphicsComponent.h"
#include <fstream>
#include <unordered_map>

#include "Camera.h"
#include "SunScript.h"

using namespace QZL;
using namespace QZL::Game;
using namespace QZL::Assets;
using namespace QZL::Graphics;

using SceneMap = std::unordered_map<std::string, std::pair<Entity*, SceneHeirarchyNode*>>;

struct SceneEntityAddInfo {
	Entity* entity = nullptr;
	std::string name = "";
	std::string parentName = "";
};

struct GameScriptAttachInfo {
	GameScript* script;
	std::string ownerName;
};

struct GraphicsComponentAttachInfo {
	GraphicsComponent* component;
	std::string ownerName;
};

GameScript* scriptLUT(const std::string& name, const SystemMasters& initialiser) {
	if (name == "CameraScript")
		return new Camera(initialiser);
	if (name == "SunScript")
		return new SunScript(initialiser);
}

SceneEntityAddInfo loadEntity(std::ifstream& in)
{
	SceneEntityAddInfo info;
	std::string token;
	in >> info.name;
	info.entity = new Entity(info.name);
	in >> token;
	ASSERT(token == "{");
	in >> token;
	in >> info.parentName;
	in >> info.entity->getTransform()->position.x;
	in >> info.entity->getTransform()->position.y;
	in >> info.entity->getTransform()->position.z;
	in >> info.entity->getTransform()->rotationAngle;
	in >> info.entity->getTransform()->rotationAxis.x;
	in >> info.entity->getTransform()->rotationAxis.y;
	in >> info.entity->getTransform()->rotationAxis.z;
	in >> info.entity->getTransform()->scale.x;
	in >> info.entity->getTransform()->scale.y;
	in >> info.entity->getTransform()->scale.z;
	in >> token;
	ASSERT(token == "}");
	return info;
}

GameScriptAttachInfo loadGameScript(const SystemMasters& masters, std::ifstream& in)
{
	GameScriptAttachInfo info;
	std::string token;
	in >> info.ownerName;
	in >> token;
	ASSERT(token == "{");
	in >> token;
	return info;
}

GraphicsComponentAttachInfo loadGraphicsComponent(std::ifstream& in)
{
}

Scene* SceneLoader::loadScene(const SystemMasters& masters, const std::string name)
{
	Scene* scene = new Scene();
	SceneMap sceneMap;
	
	std::ifstream in;
	in.open(name + ".scene");
	std::string token;
	try {
		while (in >> token && token == "ENTITY") {
			auto info = loadEntity(in);
			ASSERT(sceneMap.count(info.name) == 0);
			SceneHeirarchyNode* node;
			if (info.parentName != "") {
				node = scene->addEntity(info.entity, sceneMap[info.parentName].first, sceneMap[info.parentName].second);
			}
			else {
				node = scene->addEntity(info.entity);
			}
			sceneMap[info.name] = std::make_pair(info.entity, node);
		}

		ASSERT(token == "GAME_SCRIPT");

		do  {
			auto info = loadGameScript(masters, in);
			sceneMap[info.ownerName].first->setGameScript(info.script);
		} while (in >> token && token == "GAME_SCRIPT");

		ASSERT(token == "GRAPHICS");

		do  {
			auto info = loadGraphicsComponent(in);
			sceneMap[info.ownerName].first->setGraphicsComponent(info.component);
		} while (in >> token && token == "GRAPHICS");
	}
	catch (std::runtime_error e) {
		DEBUG_ERR("Failed to load scene");
	}

	return scene;
}
