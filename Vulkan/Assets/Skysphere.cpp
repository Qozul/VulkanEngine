#include "Skysphere.h"
#include "AltAtmosphere.h"
#include "../Graphics/Vertex.h"
#include "../Game/GameScript.h"
#include "../Game/AtmosphereScript.h"

using namespace QZL;
using namespace QZL::Game;
using Vertex = QZL::Graphics::Vertex;

Skysphere::Skysphere(const std::string name, const Graphics::LogicDevice* logicDevice, Game::SunScript* sun, Game::GameScriptInitialiser initialiser)
	: Entity(name)
{
	initialiser.owner = this;
	setGameScript(new AtmosphereScript(initialiser, sun));
	auto script = static_cast<AtmosphereScript*>(getGameScript());
	setGraphicsComponent(Graphics::RendererTypes::kAtmosphere, nullptr, script->getNewShaderParameters(), script->getMaterial(), "skysphere", loadFunction);
}

Skysphere::~Skysphere()
{
}

void Skysphere::loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices)
{
}
