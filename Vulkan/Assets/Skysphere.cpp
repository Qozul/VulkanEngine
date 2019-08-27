#include "Skysphere.h"
#include "../Graphics/AtmosphereShaderParams.h"
#include "AltAtmosphere.h"
#include "../Graphics/Vertex.h"

using namespace QZL;
using namespace Assets;
using Vertex = QZL::Graphics::Vertex;

Skysphere::Skysphere(const Graphics::LogicDevice* logicDevice, Atmosphere* atmosphere, Game::SunScript* sun)
	: atmos_(atmosphere)
{
	atmosphere->precalculateTextures(logicDevice);
	setGraphicsComponent(Graphics::RendererTypes::ATMOSPHERE, new Graphics::AtmosphereShaderParams(atmosphere->textures_, atmosphere->material_, sun), "skysphere", loadFunction);
}

Skysphere::~Skysphere()
{
	SAFE_DELETE(atmos_);
}

void Skysphere::loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::VertexOnlyPosition>& vertices)
{
	vertices = { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f) };
	indices = { 0, 1, 3, 2 };
}
