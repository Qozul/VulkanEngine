// Author: Ralph Ridley
// Date: 12/10/19

#include "Material.h"
#include "TextureManager.h"
#include "TextureSampler.h"
#include "Image.h"
#include "Descriptor.h"
#include "LogicDevice.h"
#include <fstream>
#include <sstream>

using namespace QZL;
using namespace QZL::Graphics;

const size_t Materials::materialTextureCountLUT[(size_t)MaterialType::kSize] = { 2, 3, 1, 1 };
const size_t Materials::materialSizeLUT[(size_t)MaterialType::kSize] = { sizeof(Static), sizeof(Terrain), sizeof(Atmosphere), sizeof(Particle) };

void Materials::loadMaterial(TextureManager* texManager, MaterialType type, std::string fileName, void* data)
{
	std::vector<std::string> lines;

	if (fileName != "") {
		std::ifstream file("../Data/Materials/" + fileName + ".qmat");
		ASSERT(file.is_open());
		size_t count;
		file >> count;
		lines.reserve(count + 1);
		std::string line;
		while (line != "END") {
			file >> line;
			lines.emplace_back(line);
		}
		file.close();
		(*getLoadingFunction(type))(texManager, data, lines);
	}
}

MaterialType Materials::stringToType(std::string typeName)
{
	if (typeName == "STATIC")
		return MaterialType::kStatic;
	else if (typeName == "TERRAIN")
		return MaterialType::kTerrain;
	else if (typeName == "PARTICLE")
		return MaterialType::kParticle;
	else if (typeName == "ATMOSPHERE")
		return MaterialType::kAtmosphere;
}

Materials::MaterialLoadingFunction Materials::getLoadingFunction(MaterialType type)
{
	switch (type) {
	case MaterialType::kStatic:
		return Materials::loadStaticMaterial;
	case MaterialType::kTerrain:
		return Materials::loadTerrainMaterial;
	case MaterialType::kParticle:
		return Materials::loadParticleMaterial;
	default:
		ASSERT(false);
	}
}

void Materials::loadStaticMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 2);
	Static material = {};
	material.albedoIdx = texManager->requestTexture(lines[0]);
	material.normalmapIdx = texManager->requestTexture(lines[1]);
	memcpy(data, &material, sizeof(Static));
}

void Materials::loadTerrainMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 3);
	Terrain material = {};
	material.heightmapIdx = texManager->requestTexture(lines[0]);
	material.normalmapIdx = texManager->requestTexture(lines[1]);
	material.albedoIdx = texManager->requestTexture(lines[2]);
	memcpy(data, &material, sizeof(Terrain));
}

void Materials::loadParticleMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 1);
	Particle material = {};
	material.albedoIdx = texManager->requestTexture(lines[0]);
	memcpy(data, &material, sizeof(Atmosphere));
}
