#include "AssetManager.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Physics/RigidBody.h"
#include "../Physics/CollisionVolume.h"
#include "../Game/GameScript.h"
#include "../Graphics/MeshLoader.h"
#include "../Graphics/TextureLoader.h"

using namespace QZL::Assets;

AssetManager::AssetManager()
	: meshLoader(new Graphics::MeshLoader()), textureLoader(nullptr)
{
}

AssetManager::~AssetManager()
{
	delete meshLoader;
	SAFE_DELETE(textureLoader);
	for (auto entity : entities_) {
		SAFE_DELETE(entity);
	}
}

Entity* AssetManager::createEntity()
{
	Entity* entity = new Entity();
	entities_.push_back(entity);
	return entity;
}
