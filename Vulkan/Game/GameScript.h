#pragma once
#include "../Assets/Transform.h"
#include "../SystemMasters.h"

namespace QZL {
	class InputManager;
	class System;
	class Entity;
	namespace Graphics {
		struct BasicMesh;
	}
	namespace Game {
		struct GameScriptInitialiser {
			Entity* owner;
			InputManager* inputManager;
			System* system;
		};
		class GameScript {
			friend class GameMaster;
			friend class Entity;
		protected:
			GameScript(const GameScriptInitialiser& initialiser);
			GameScript(const SystemMasters& initialiser);
			virtual ~GameScript() { }

			virtual void start() = 0;
			virtual void update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix) = 0;
			// Some scripts will create meshes differently to the default and will use this to make construct the mesh for graphics component.
			virtual Graphics::BasicMesh* makeMesh() { return nullptr; }
			Transform* transform();

			Entity* owningEntity_;
			InputManager* inputManager_;
			const SystemMasters* sysMasters_;
		};
	}
}
