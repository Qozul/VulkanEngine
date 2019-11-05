#include "Terrain.h"
#include "../Graphics/ShaderParams.h"
#include "../Graphics/Material.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/Vertex.h"
#include "Transform.h"

using namespace QZL;
using namespace Assets;
using namespace Graphics;

Terrain::Terrain(const std::string name, TextureManager* textureManager)
	: Entity(name)
{
	setGraphicsComponent(Graphics::RendererTypes::kTerrain, new TerrainShaderParams(glm::vec3(1.0f), glm::vec3(0.8f), 1.0f, 10.0f), nullptr,
		textureManager->requestMaterial<TerrainMaterial>("ExampleTerrain"), "terrain", loadFunction);
	transform_->position.y = 100.0f;
}

void Terrain::loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices)
{
	const int gridSize = 1024;
	const int numSubGrids = 100;
	const int subGridSize = gridSize / numSubGrids;

	std::vector<uint16_t> inds;
	std::vector<Graphics::Vertex> verts;
	// Vertex grid
	for (int x = 0; x < numSubGrids; ++x) {
		for (int z = 0; z < numSubGrids; ++z) {
			float vx = static_cast<float>(x * subGridSize);
			float vz = static_cast<float>(z * subGridSize);
			float u = vx / gridSize;
			float v = vz / gridSize;
			verts.emplace_back(vx, 0.0f, vz, u, v, 0.0f, 1.0f, 0.0f);
		}
	}
	// Form in to quad patches with indices
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
