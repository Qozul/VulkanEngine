#include "Water.h"
#include "../Graphics/ShaderParams.h"
#include "../Graphics/Material.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/Vertex.h"
#include "../Graphics/TextureLoader.h"
#include "Transform.h"

using namespace QZL;
using namespace Graphics;

Water::Water(const std::string name, TextureManager* textureManager)
	: Entity(name)
{
	setGraphicsComponent(Graphics::RendererTypes::kWater, nullptr, new WaterShaderParams(glm::vec4(0.25f, 0.64f, 0.87f, 1.0f), glm::vec4(0.8f, 0.8f, 0.8f, 100.0f)),
		textureManager->requestMaterial(Graphics::RendererTypes::kWater, "Water"), "water", loadFunction);
}

void Water::update(float dt, const glm::mat4& viewProjection, const glm::mat4& parentMatrix)
{
	auto params = static_cast<WaterShaderParams*>(graphicsComponent_->getShaderParams());
	params->baseColour.w += dt * 0.01f;
	if (params->baseColour.w > 1.0f) {
		params->baseColour.w = 0.0f;
	}
}

void Water::loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices)
{
	const int gridSize = 1000;
	const int numSubGrids = 50;
	const int subGridSize = gridSize / numSubGrids;
	std::vector<uint16_t> inds;
	std::vector<Graphics::Vertex> verts;
	for (int x = 0; x < numSubGrids; ++x) {
		for (int z = 0; z < numSubGrids; ++z) {
			float vx = static_cast<float>(x * subGridSize);
			float vz = static_cast<float>(z * subGridSize);
			float u = vx;
			float v = vz;
			verts.emplace_back(vx, 0.0f, vz, u / gridSize, v / gridSize, 0.0f, 0.0f, 0.0f);
		}
	}
	for (int x = 0; x < numSubGrids - 1; ++x) {
		for (int z = 0; z < numSubGrids - 1; ++z) {
			int xoffset0 = x * numSubGrids;
			int xoffset1 = (x + 1) * numSubGrids;
			inds.push_back(xoffset0 + z);
			inds.push_back(xoffset0 + z + 1);
			inds.push_back(xoffset1 + z + 1);
			inds.push_back(xoffset1 + z);
		}
	}
	count = static_cast<uint32_t>(inds.size());
	indices.resize(inds.size() * sizeof(uint16_t));
	vertices.resize(verts.size() * sizeof(Graphics::Vertex));
	memcpy(indices.data(), inds.data(), inds.size() * sizeof(uint16_t));
	memcpy(vertices.data(), verts.data(), verts.size() * sizeof(Graphics::Vertex));
}
