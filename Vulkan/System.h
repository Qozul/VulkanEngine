/*
Author: Ralph Ridley
Date: 03/07/19

The main system class which controls all subsystems.
*/
#pragma once
#include "../Shared/Utility.h"

int main(int argc, char** argv);

namespace QZL {
	namespace Assets {
		class AssetManager;
	}
	namespace Graphics { 
		class GraphicsMaster;
	}
	namespace Physics {
		class PhysicsMaster;
	}
	namespace Game {
		class GameMaster;
	}
	class System;
	class InputManager;
	struct SystemMasters {
		System* system;
		InputManager* inputManager;
		Game::GameMaster* gameMaster;
		Physics::PhysicsMaster* physicsMaster;
		Graphics::GraphicsMaster* graphicsMaster;
		Assets::AssetManager* assetManager;
	};

	class System final {
		friend int ::main(int argc, char** argv);
	public:
		System();
		~System();
		void loop();
		const SystemMasters& getMasters() {
			return masters_;
		}
		static float deltaTime;
		static float deltaTimeSeconds;
	private:
		SystemMasters masters_;
		InputManager* inputManager_;
	};
}
