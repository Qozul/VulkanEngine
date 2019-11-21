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
	setGraphicsComponent(Graphics::RendererTypes::kTerrain, nullptr, new TerrainShaderParams(200.0f, 0.0f, 0.3f, 0.9f),
		textureManager->requestMaterial(Graphics::RendererTypes::kTerrain, "Terrain"), "terrain", loadFunction);
}

void Terrain::loadFunction(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices)
{
	auto heightmap = TextureLoader::getCPUImage("Heightmaps/hmap1.png", 1024, 1024, 1, 1);
	const int gridSize = 1024;
	const int numSubGrids = 150;
	const int subGridSize = gridSize / numSubGrids;
	std::vector<uint16_t> inds;
	std::vector<Graphics::Vertex> verts;
	for (int x = 0; x < numSubGrids; ++x) {
		for (int z = 0; z < numSubGrids; ++z) {
			int expandedX = x * subGridSize;
			int expandedZ = z * subGridSize;
			float vx = static_cast<float>(expandedX);
			float vz = static_cast<float>(expandedZ);
			unsigned char rawHeight = heightmap[expandedX + expandedZ * 1024];
			float vy = ((float)rawHeight / 256.0f) * 200.0f;
			float u = x;
			float v = z;

			expandedX = expandedX == 0 ? 1 : expandedX == 1024 ? 1023 : expandedX;
			expandedZ = expandedZ == 0 ? 1 : expandedZ == 1024 ? 1023 : expandedZ;
			float hu = ((float)heightmap[expandedX + (expandedZ + 1) * 1024] / 256.0f) * 200.0f;
			float hd = ((float)heightmap[expandedX + (expandedZ - 1) * 1024] / 256.0f) * 200.0f;
			float hl = ((float)heightmap[expandedX - 1 + expandedZ * 1024] / 256.0f) * 200.0f;
			float hr = ((float)heightmap[expandedX + 1 + expandedZ * 1024] / 256.0f) * 200.0f;

			glm::vec3 normal = glm::vec3(hl - hr, 2.0f, hd - hu);
			normal = glm::normalize(normal);

			verts.emplace_back(vx, vy, vz, u, v, normal.x, normal.y, normal.z);
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
	TextureLoader::freeCPUImage(heightmap);
	count = static_cast<uint32_t>(inds.size());
	indices.resize(inds.size() * sizeof(uint16_t));
	vertices.resize(verts.size() * sizeof(Graphics::Vertex));
	memcpy(indices.data(), inds.data(), inds.size() * sizeof(uint16_t));
	memcpy(vertices.data(), verts.data(), verts.size() * sizeof(Graphics::Vertex));
}
