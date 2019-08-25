#pragma once

namespace QZL {
	class System;
	class InputManager;
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
	struct SystemMasters {
		System* system;
		InputManager* inputManager;
		Game::GameMaster* gameMaster;
		Physics::PhysicsMaster* physicsMaster;
		Graphics::GraphicsMaster* graphicsMaster;
		Assets::AssetManager* assetManager;
	};
}
