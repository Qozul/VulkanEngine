#include "Skysphere.h"
#include "../Graphics/AtmosphereShaderParams.h"
#include "AltAtmosphere.h"
#include "../Graphics/Vertex.h"
#include "../Game/GameScript.h"
#include "../Game/AtmosphereScript.h"

using namespace QZL;
using namespace Assets;
using namespace QZL::Game;
using Vertex = QZL::Graphics::Vertex;

Skysphere::Skysphere(const std::string name, const Graphics::LogicDevice* logicDevice, Game::SunScript* sun, Game::GameScriptInitialiser initialiser)
	: Entity(name)
{
	initialiser.owner = this;
	setGameScript(new AtmosphereScript(initialiser));
	auto script = static_cast<AtmosphereScript*>(getGameScript());
	setGraphicsComponent(Graphics::RendererTypes::ATMOSPHERE, new Graphics::AtmosphereShaderParams(script->getTextures(), script->getMaterial(), sun), "skysphere", loadFunction);
}

Skysphere::~Skysphere()
{
}

void Skysphere::loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices)
{
	vertices = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) };
	indices = { 0, 1, 3, 2 };
}
