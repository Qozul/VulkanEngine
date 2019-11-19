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

const size_t Materials::materialTextureCountLUT[(size_t)RendererTypes::kNone] = { 2, 3, 1, 1 };
const size_t Materials::materialSizeLUT[(size_t)RendererTypes::kNone] = { sizeof(Static), sizeof(Terrain), sizeof(Atmosphere), sizeof(Particle), sizeof(PostProcess) };

void Materials::loadMaterial(TextureManager* texManager, RendererTypes type, std::string fileName, void* data)
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

RendererTypes Materials::stringToType(std::string typeName)
{
	if (typeName == "STATIC")
		return RendererTypes::kStatic;
	else if (typeName == "TERRAIN")
		return RendererTypes::kTerrain;
	else if (typeName == "PARTICLE")
		return RendererTypes::kParticle;
	else if (typeName == "ATMOSPHERE")
		return RendererTypes::kAtmosphere;
	else if (typeName == "POST_PROCESS")
		return RendererTypes::kPostProcess;
	else if (typeName == "WATER")
		return RendererTypes::kWater;
}

Materials::MaterialLoadingFunction Materials::getLoadingFunction(RendererTypes type)
{
	switch (type) {
	case RendererTypes::kStatic:
		return Materials::loadStaticMaterial;
	case RendererTypes::kTerrain:
		return Materials::loadTerrainMaterial;
	case RendererTypes::kParticle:
		return Materials::loadParticleMaterial;
	case RendererTypes::kWater:
		return Materials::loadWaterMaterial;
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
	material.normalmapIdx = texManager->requestTexture(lines[2]);
	material.albedoIdx = texManager->requestTexture(lines[1]);
	//material.detailNormalmapIdx = texManager->requestTexture(lines[3]);
	memcpy(data, &material, sizeof(Terrain));
}

void Materials::loadParticleMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 1);
	Particle material = {};
	material.albedoIdx = texManager->requestTexture(lines[0]);
	memcpy(data, &material, sizeof(Particle));
}

void Materials::loadWaterMaterial(TextureManager* texManager, void* data, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 3);
	Water material = {};
	material.displacementMap = texManager->requestTexture(lines[0]);
	material.normalMap = texManager->requestTexture(lines[1]);
	material.specularMap = texManager->requestTexture(lines[2]);
	memcpy(data, &material, sizeof(Water));
}
