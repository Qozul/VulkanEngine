#pragma once
#include "Entity.h"

namespace QZL {
	namespace Game {
		class SunScript;
		struct GameScriptInitialiser;
	}
	class Skysphere : public Entity {
	public:
		Skysphere(const std::string name, const Graphics::LogicDevice* logicDevice, Game::SunScript* sun, Game::GameScriptInitialiser initialiser);
		~Skysphere();
	private:
		static void loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices);
	};
}
