#pragma once

namespace QZL {
	class System;
	class InputManager;
	namespace Graphics {
		class GraphicsMaster;
		class TextureManager;
		class LogicDevice;
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
		Graphics::TextureManager* textureManager;

		const Graphics::LogicDevice* getLogicDevice() const;
	};
}
