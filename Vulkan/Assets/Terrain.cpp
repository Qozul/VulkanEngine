#include "Terrain.h"
#include "../Graphics/ShaderParams.h"
#include "../Graphics/Material.h"
#include "../Graphics/LogicDevice.h"
#include "../Graphics/TextureManager.h"
#include "Transform.h"

using namespace QZL;
using namespace Assets;
using namespace Graphics;

Terrain::Terrain(const std::string name, TextureManager* textureManager)
	: Entity(name)
{
	setGraphicsComponent(Graphics::RendererTypes::TERRAIN, new TerrainShaderParams(glm::vec3(1.0f), glm::vec3(0.8f), 1.0f, 10.0f), nullptr,
		textureManager->requestMaterial<TerrainMaterial>("ExampleTerrain"), "terrain", Graphics::MeshLoadingInfo(loadFunction));
	transform_->position.y = 100.0f;
}

void Terrain::loadFunction(std::vector<Graphics::IndexType>& indices, std::vector<Graphics::Vertex>& vertices)
{
	const int gridSize = 1024;
	const int numSubGrids = 100;
	const int subGridSize = gridSize / numSubGrids;

	// Vertex grid
	for (int x = 0; x < numSubGrids; ++x) {
		for (int z = 0; z < numSubGrids; ++z) {
			float vx = x * subGridSize;
			float vz = z * subGridSize;
			float u = vx / gridSize;
			float v = vz / gridSize;
			vertices.emplace_back(vx, 0.0f, vz, u, v, 0.0f, 1.0f, 0.0f);
		}
	}
	// Form in to quad patches with indices
	for (int x = 0; x < numSubGrids - 1; ++x) {
		for (int z = 0; z < numSubGrids - 1; ++z) {
			int xoffset0 = x * numSubGrids;
			int xoffset1 = (x + 1) * numSubGrids;
			indices.push_back(xoffset0 + z);
			indices.push_back(xoffset0 + z + 1);
			indices.push_back(xoffset1 + z + 1);
			indices.push_back(xoffset1 + z);
		}
	}
}
