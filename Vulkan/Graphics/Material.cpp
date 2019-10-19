// Author: Ralph Ridley
// Date: 12/10/19

#include "Material.h"
#include "TextureManager.h"
#include "TextureSampler.h"

using namespace QZL;
using namespace QZL::Graphics;

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


VkDescriptorSetLayout StaticMaterial::getLayout(Descriptor* descriptor)
{
	// TODO
	return VkDescriptorSetLayout();
}

std::vector<TextureSampler*> StaticMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	// TODO
	return {};
}

VkDescriptorSetLayout StaticMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
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
	return descriptor->makeLayout({ makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr) });
}

std::vector<TextureSampler*> AtmosphereMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 1);
	scatteringTexture_ = textureManager->requestTextureSeparate(lines[0]);
	return { scatteringTexture_ };
}

VkDescriptorSetLayout QZL::Graphics::AtmosphereMaterial::makeLayout(Descriptor* descriptor)
{
	return getLayout(descriptor);
}
