/*
Author: Ralph Ridley
Date: 01/07/19

Manage entities, textures, and models.
*/
#pragma once
#include "../../Shared/Utility.h"
#include "Entity.h"

namespace QZL {
	namespace Graphics {
		class MeshLoader;
		class TextureLoader;
		class TextureManager;
	}
	namespace Assets {
		class AssetManager final {
			friend class System;
		public:
			Entity* createEntity();
			template<typename T>
			Entity* createEntity();

			Graphics::MeshLoader* meshLoader;
			Graphics::TextureManager* textureManager;
		private:
			AssetManager();
			~AssetManager();
			std::vector<Entity*> entities_;
		};

		template<typename T>
		Entity* AssetManager::createEntity()
		{
			Entity* entity = new T();
			entities_.push_back(entity);
			return entity;
		}
	}
}
