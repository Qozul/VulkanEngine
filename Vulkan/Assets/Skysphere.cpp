#include "Skysphere.h"
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
	setGameScript(new AtmosphereScript(initialiser, sun));
	auto script = static_cast<AtmosphereScript*>(getGameScript());
	setGraphicsComponent(Graphics::RendererTypes::ATMOSPHERE, script->getNewShaderParameters(), nullptr, script->getMaterial(), "skysphere", loadFunction);
}

Skysphere::~Skysphere()
{
}

void Skysphere::loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices)
{
	std::vector<Graphics::IndexType> inds = { 0, 1, 3, 2 };
	std::vector<Graphics::VertexOnlyPosition> verts = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) };
	count = static_cast<uint32_t>(inds.size());
	indices.resize(inds.size() * sizeof(Graphics::IndexType));
	vertices.resize(verts.size() * sizeof(Graphics::VertexOnlyPosition));
	memcpy(indices.data(), inds.data(), inds.size() * sizeof(Graphics::IndexType));
	memcpy(vertices.data(), verts.data(), verts.size() * sizeof(Graphics::VertexOnlyPosition));
}
