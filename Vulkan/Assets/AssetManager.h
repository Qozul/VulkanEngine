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
	}
	namespace Assets {
		class AssetManager final {
			friend class System;
		public:
			// Allocates and creates a new entity model, with components created depending on the flags.
			Entity* createEntity();

			Graphics::MeshLoader* meshLoader;
			Graphics::TextureLoader* textureLoader;
		private:
			AssetManager();
			~AssetManager();
			std::vector<Entity*> entities_;
		};
	}
}
