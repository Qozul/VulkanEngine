/*
Author: Ralph Ridley
Date: 01/07/19

Manage entities, textures, and models.
*/
#pragma once
#include "../../Shared/Utility.h"
#include "../SystemMasters.h"
#include "Entity.h"

namespace QZL {
	struct SystemMasters;
	namespace Graphics {
		class MeshLoader;
		class TextureLoader;
		class TextureManager;
		class LogicDevice;
	}
	namespace Assets {
		class AssetManager final {
			friend class System;
		public:
			Entity* createEntity(const std::string name);
			template<typename T, typename... Args>
			Entity* createEntity(const std::string name, Args&& ... args);

			void deleteEntity(Assets::Entity* entity);

			Graphics::MeshLoader* meshLoader;
			Graphics::TextureManager* textureManager;
		private:
			AssetManager(const SystemMasters& masters);
			~AssetManager();
			std::vector<Entity*> entities_;

			// Only uses masters to reference the game master to clean up scene correctly when deleting an entity
			// With this assumption, no nullptr checks are needed as entities are only added once the game master is
			// created.
			const SystemMasters& masters_;
		};

		template<typename T, typename... Args>
		Entity* AssetManager::createEntity(const std::string name, Args&&... args)
		{
			Entity* entity = new T(name, std::forward<Args>(args)...);
			entities_.push_back(entity);
			return entity;
		}
	}
}
