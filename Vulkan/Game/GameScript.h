#pragma once

namespace QZL {
	class Entity;
	class InputManager;
	class System;
	namespace Game {
		struct GameScriptInitialiser {
			Entity* owner;
			InputManager* inputManager;
			System* system;
		};
		class GameScript {
			friend class GameMaster;
		protected:
			GameScript(const GameScriptInitialiser& initialiser)
				: owningEntity_(initialiser.owner), inputManager_(initialiser.inputManager) { }

			virtual void start() = 0;
			virtual void update() = 0;

			Entity* owningEntity_;
			InputManager* inputManager_;
		};
	}
}
