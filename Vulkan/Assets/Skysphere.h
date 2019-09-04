#pragma once
#include "Entity.h"

namespace QZL {
	namespace Game {
		class SunScript;
		struct GameScriptInitialiser;
	}
	namespace Assets {
		class Atmosphere;
		class Skysphere : public Entity {
		public:
			Skysphere(const Graphics::LogicDevice* logicDevice, Game::SunScript* sun, Game::GameScriptInitialiser initialiser);
			~Skysphere();
		private:
			Atmosphere* atmos_;

			static void loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices);
		};
	}
}
