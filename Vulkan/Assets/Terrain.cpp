#include "Terrain.h"
#include "../Graphics/ShaderParams.h"
#include "../Graphics/Material.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/Vertex.h"
#include "../Graphics/TextureLoader.h"
#include "Transform.h"

using namespace QZL;
using namespace Graphics;

Terrain::Terrain(const std::string name, TextureManager* textureManager)
	: Entity(name)
{
	setGraphicsComponent(Graphics::RendererTypes::kTerrain, nullptr, new TerrainShaderParams(glm::vec3(1.5f), glm::vec3(0.8f), 1.0f, 10.0f),
		textureManager->requestMaterial(Graphics::RendererTypes::kTerrain, "ExampleTerrain"), "terrain", loadFunction);
}

void Terrain::loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices)
{
	const int gridSize = 1024;
	const int numSubGrids = 200;
	const int subGridSize = gridSize / numSubGrids;
	std::vector<uint16_t> inds;
	std::vector<Graphics::Vertex> verts;
	for (int x = 0; x < numSubGrids; ++x) {
		for (int z = 0; z < numSubGrids; ++z) {
			float vx = static_cast<float>(x * subGridSize);
			float vz = static_cast<float>(z * subGridSize);
			float u = vx;
			float v = vz;
			// Store normalized coordinates where the normals go, and the maximum height in the final normal
			verts.emplace_back(vx, 0.0f, vz, u, v, vx / 1024.0f, vz / 1024.0f, 200.0f);
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
