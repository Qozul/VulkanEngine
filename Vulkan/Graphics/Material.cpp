// Author: Ralph Ridley
// Date: 12/10/19

#include "Material.h"
#include "TextureManager.h"
#include "TextureSampler.h"

using namespace QZL;
using namespace QZL::Graphics;

std::vector<VkDescriptorSetLayoutBinding> ParticleMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 1);
	diffuse_ = textureManager->requestTextureSeparate(lines[0]);
	return { makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, diffuse_->getSampler()) };
}

std::vector<VkDescriptorSetLayoutBinding> StaticMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	// TODO
	return { makeLayoutBinding(0, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, nullptr) };
}

std::vector<VkDescriptorSetLayoutBinding> TerrainMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 2);
	heightmap_ = textureManager->requestTextureSeparate(lines[0]);
	diffuse_ = textureManager->requestTextureSeparate(lines[1]);
	return {
		makeLayoutBinding(0, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, heightmap_->getSampler()),
		makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, diffuse_->getSampler())
	};
}

std::vector<VkDescriptorSetLayoutBinding> AtmosphereMaterial::loadTextures(TextureManager* textureManager, std::vector<std::string>& lines)
{
	ASSERT(lines.size() >= 1);
	scatteringTexture_ = textureManager->requestTextureSeparate(lines[0]);
	return { makeLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, scatteringTexture_->getSampler()) };
}
