#pragma once
#include "../Assets/Transform.h"

namespace QZL {
	class InputManager;
	class System;
	namespace Assets {
		class Entity;
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
			GameScript(const GameScriptInitialiser& initialiser)
				: owningEntity_(initialiser.owner), inputManager_(initialiser.inputManager) { }

			virtual void start() = 0;
			virtual void update(float dt) = 0;
			Transform* transform();

			Assets::Entity* owningEntity_;
			InputManager* inputManager_;
		};
	}
}
