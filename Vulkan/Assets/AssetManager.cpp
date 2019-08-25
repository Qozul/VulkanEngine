#include "AssetManager.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Physics/RigidBody.h"
#include "../Physics/CollisionVolume.h"
#include "../Game/GameScript.h"
#include "../Game/GameMaster.h"
#include "../Game/Scene.h"
#include "../Graphics/MeshLoader.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/LogicDevice.h"

using namespace QZL;
using namespace QZL::Assets;

AssetManager::AssetManager(const SystemMasters& masters)
	: meshLoader(new Graphics::MeshLoader()), textureManager(nullptr), masters_(masters)
{
}

AssetManager::~AssetManager()
{
	delete meshLoader;
	SAFE_DELETE(textureManager);
	for (auto entity : entities_) {
		SAFE_DELETE(entity);
	}
}

Entity* AssetManager::createEntity() 
{
	return createEntity<Entity>();
}

void AssetManager::deleteEntity(Entity* entity)
{
	if (entity->getSceneNode() != nullptr) {
		masters_.gameMaster->getActiveScene()->removeEntity(entity, entity->preserveChildrenOnDelete_, entity->getSceneNode());
	}
	entities_.erase(std::find(entities_.begin(), entities_.end(), entity));
	SAFE_DELETE(entity);
}
