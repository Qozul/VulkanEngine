#include "AssetManager.h"
#include "../Graphics/GraphicsComponent.h"
#include "../Physics/RigidBody.h"
#include "../Physics/CollisionVolume.h"
#include "../Game/GameScript.h"
#include "../Graphics/MeshLoader.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/LogicDevice.h"

using namespace QZL::Assets;

AssetManager::AssetManager()
	: meshLoader(new Graphics::MeshLoader()), textureManager(nullptr)
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
