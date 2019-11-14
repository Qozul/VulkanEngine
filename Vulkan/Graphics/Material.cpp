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




/*

const size_t Material::materialSizeLUT[] = { sizeof(StaticMaterial), sizeof(TerrainMaterial), sizeof(AtmosphereMaterial), sizeof(ParticleMaterial), 0 };

void Material::load(TextureManager* textureManager, Descriptor* descriptor)
{
	std::vector<std::string> lines;
	readFile(lines);
	layout_ = makeLayout(descriptor);
	makeTextureSet(descriptor, loadTextures(textureManager, lines));
}

void Material::readFile(std::vector<std::string>& lines)
{
	// Validation is deferred
	std::ifstream file("../Data/Materials/" + materialFileName_ + ".qmat");
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
}

void Material::makeTextureSet(Descriptor* descriptor, std::vector<TextureSampler*> samplers)
{
	if (samplers.size() > 0) {
		textureSet_ = descriptor->getSet(descriptor->createSets({ layout_ }));
		std::vector<VkWriteDescriptorSet> setWrites(samplers.size());
		for (size_t i = 0; i < samplers.size(); ++i) {
			setWrites[i] = samplers[i]->descriptorWrite(textureSet_, static_cast<uint32_t>(i));
		}
		descriptor->updateDescriptorSets(setWrites);
	}
}

constexpr VkDescriptorSetLayoutBinding QZL::Graphics::Material::makeLayoutBinding(uint32_t idx, VkShaderStageFlags stageFlags, VkSampler* sampler, VkDescriptorType type)
{
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = idx;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = type;
	layoutBinding.pImmutableSamplers = sampler;
	layoutBinding.stageFlags = stageFlags;
	return layoutBinding;
}

ParticleMaterial::~ParticleMaterial() 
{
}

VkDescriptorSetLayout ParticleMaterial::getLayout(Descriptor* descriptor)
{
	return descriptor->makeLayout({ makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr) });
}

std::vector<TextureSampler*> ParticleMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 1);
	diffuse_ = textureManager->requestTexture(lines[0]);
	return { };
}

VkDescriptorSetLayout ParticleMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
}

StaticMaterial::~StaticMaterial()
{
}

VkDescriptorSetLayout StaticMaterial::getLayout(Descriptor* descriptor)
{
	return descriptor->makeLayout({ makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr), makeLayoutBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr) });
}

std::vector<TextureSampler*> StaticMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 2);
	diffuseTextureIndex = textureManager->requestTexture(lines[0]);
	normalMapIndex = textureManager->requestTexture(lines[1]);
	return {};
}

VkDescriptorSetLayout StaticMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
}


TerrainMaterial::~TerrainMaterial()
{
}

VkDescriptorSetLayout TerrainMaterial::getLayout(Descriptor* descriptor)
{
	return descriptor->makeLayout({
		makeLayoutBinding(0, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, nullptr),
		makeLayoutBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr),
		makeLayoutBinding(2, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, nullptr)
	});
}

std::vector<TextureSampler*> TerrainMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 3);
	heightmap_ = textureManager->requestTexture(lines[0], SamplerInfo(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT));
	diffuse_ = textureManager->requestTexture(lines[1]);
	normalmap_ = textureManager->requestTexture(lines[2], SamplerInfo(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT));
	return {  };
}

VkDescriptorSetLayout TerrainMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
}


VkDescriptorSetLayout AtmosphereMaterial::getLayout(Descriptor* descriptor)
{
	return descriptor->makeLayout({ makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr) });
}

VkDescriptorSetLayout AtmosphereMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
}
*/