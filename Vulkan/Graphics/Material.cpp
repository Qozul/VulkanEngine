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
	SAFE_DELETE(diffuse_);
}

VkDescriptorSetLayout ParticleMaterial::getLayout(Descriptor* descriptor)
{
	return descriptor->makeLayout({ makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr) });
}

std::vector<TextureSampler*> ParticleMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 1);
	diffuse_ = textureManager->requestTextureSeparate(lines[0]);
	return { diffuse_ };
}

VkDescriptorSetLayout ParticleMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
}

StaticMaterial::~StaticMaterial()
{
	if (!isUsingDI) {
		SAFE_DELETE(diffuse_.diffuseSampler);
		SAFE_DELETE(normalMap_.normalMapSampler);
	}
}

VkDescriptorSetLayout StaticMaterial::getLayout(Descriptor* descriptor)
{
	return descriptor->makeLayout({ makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr), makeLayoutBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr) });
}

std::vector<TextureSampler*> StaticMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	isUsingDI = textureManager->descriptorIndexingEnabled();
	ASSERT(lines.size() >= 2);
	if (isUsingDI) {
		diffuse_.diffuseTextureIndex = textureManager->requestTexture(lines[0]);
		normalMap_.normalMapIndex = textureManager->requestTexture(lines[1]);
		return {};
	}
	else {
		diffuse_.diffuseSampler = textureManager->requestTextureSeparate(lines[0]);
		normalMap_.normalMapSampler = textureManager->requestTextureSeparate(lines[1]);
		return { diffuse_.diffuseSampler, normalMap_.normalMapSampler };
	}
}

VkDescriptorSetLayout StaticMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
}


TerrainMaterial::~TerrainMaterial()
{
	SAFE_DELETE(heightmap_);
	SAFE_DELETE(diffuse_);
}

VkDescriptorSetLayout TerrainMaterial::getLayout(Descriptor* descriptor)
{
	return descriptor->makeLayout({
		makeLayoutBinding(0, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, nullptr),
		makeLayoutBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr)
	});
}

std::vector<TextureSampler*> TerrainMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 2);
	heightmap_ = textureManager->requestTextureSeparate(lines[0]);
	diffuse_ = textureManager->requestTextureSeparate(lines[1]);
	return { heightmap_, diffuse_ };
}

VkDescriptorSetLayout TerrainMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
}


VkDescriptorSetLayout AtmosphereMaterial::getLayout(Descriptor* descriptor)
{
	return descriptor->makeLayout({ makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr), makeLayoutBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr) });
}

VkDescriptorSetLayout AtmosphereMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
}
