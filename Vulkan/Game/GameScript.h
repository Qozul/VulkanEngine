#pragma once
#include "../Assets/Transform.h"
#include "../SystemMasters.h"

namespace QZL {
	class InputManager;
	class System;
	namespace Assets {
		class Entity;
	}
	namespace Graphics {
		struct BasicMesh;
	}
	namespace Game {
		struct GameScriptInitialiser {
			Assets::Entity* owner;
			InputManager* inputManager;
			System* system;
		};
		class GameScript {
			friend class GameMaster;
			friend class Assets::Entity;
		protected:
			GameScript(const GameScriptInitialiser& initialiser);
			virtual ~GameScript() { }

			virtual void start() = 0;
			virtual void update(float dt) = 0;
			// Some scripts will create meshes differently to the default and will use this to make construct the mesh for graphics component.
			virtual Graphics::BasicMesh* makeMesh() { return nullptr; }
			Transform* transform();

			Assets::Entity* owningEntity_;
			InputManager* inputManager_;
			const SystemMasters* sysMasters_;
		};
	}
}
