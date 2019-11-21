// Author: Ralph Ridley
// Date: 01/11/19
#include "TextureManager.h"
#include "Descriptor.h"
#include "TextureLoader.h"
#include "TextureSampler.h"
#include "Image.h"
#include "GlobalRenderData.h"

using namespace QZL;
using namespace Graphics;

TextureManager::TextureManager(const LogicDevice* logicDevice, Descriptor* descriptor, uint32_t maxTextures, bool descriptorIndexing)
	: logicDevice_(logicDevice), descriptorIndexingActive_(descriptorIndexing), maxTextures_(maxTextures), textureLoader_(new TextureLoader(logicDevice)),
	descriptorSetIdx_(0), descriptor_(descriptor)
{
	setLayoutBinding_ = {};
	setLayoutBinding_.binding = (uint32_t)GlobalRenderDataBindings::kTextureArray;
	setLayoutBinding_.descriptorCount = maxTextures;
	setLayoutBinding_.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setLayoutBinding_.pImmutableSamplers = nullptr;
	setLayoutBinding_.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

	for (uint32_t i = 0; i < maxTextures; ++i) {
		freeDescriptors_.push(i);
	}

	materialCount_ = 0;
	materialData_.resize(1000);
}

TextureManager::~TextureManager()
{
	SAFE_DELETE(textureLoader_);
	for (auto it : materials_) {
		SAFE_DELETE(it.second);
	}
	for (auto it : textures_) {
		SAFE_DELETE(it.second);
	}
	for (auto it : textureSamplersDI_) {
		SAFE_DELETE(it.second.first);
	}
}

uint32_t TextureManager::requestTexture(const std::string& name, SamplerInfo samplerInfo)
{
	// Reuse sampler if it already exists
	if (textureSamplersDI_.count(name)) {
		return textureSamplersDI_[name].second;
	}
	else {
		auto sampler = requestTextureSeparate(name, samplerInfo.magFilter, samplerInfo.minFilter, samplerInfo.addressMode, samplerInfo.anisotropy);
		textureSamplersDI_[name].first = sampler;

		uint32_t arrayIdx = freeDescriptors_.front();
		freeDescriptors_.pop();

		descriptor_->updateDescriptorSets({ makeDescriptorWrite(sampler->getImageInfo(), arrayIdx, 1) });

		textureSamplersDI_[name].second = arrayIdx;
		return arrayIdx;
	}
}

TextureSampler* TextureManager::requestTextureSeparate(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode,
	float anisotropy, VkShaderStageFlags stages)
{
	// Always make a new sampler, but reuse image if it exists
	if (textures_.count(name)) {
		return textures_[name]->createTextureSampler(name, magFilter, minFilter, addressMode, anisotropy);
	}
	else {
		auto image = textureLoader_->loadTexture(name, stages);
		textures_[name] = image;
		return image->createTextureSampler(name, magFilter, minFilter, addressMode, anisotropy);
	}
}
TextureSampler* TextureManager::requestTextureSeparate(const std::array<std::string, 6U> names, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode,
	float anisotropy, VkShaderStageFlags stages)
{
	std::string keyName = names[0] + names[1] + names[2] + names[3] + names[4] + names[5];
	if (textures_.count(keyName)) {
		return textures_[keyName]->createTextureSampler(keyName, magFilter, minFilter, addressMode, anisotropy);
	}
	else {
		auto image = textureLoader_->loadCubeTexture(names, stages);
		textures_[keyName] = image;
		return image->createTextureSampler(keyName, magFilter, minFilter, addressMode, anisotropy);
	}
}

uint32_t TextureManager::allocateTexture(const std::string& name, Image*& imgPtr, VkImageCreateInfo createInfo, MemoryAllocationPattern allocationPattern, ImageParameters parameters, SamplerInfo samplerInfo)
{
	imgPtr = allocateImage(name, createInfo, allocationPattern, parameters);
	textures_[name] = imgPtr;
	return allocateTexture(name, imgPtr, samplerInfo);
}

uint32_t TextureManager::allocateTexture(const std::string& name, Image* img, SamplerInfo samplerInfo)
{
	uint32_t arrayIdx = freeDescriptors_.front();
	freeDescriptors_.pop();
	TextureSampler* sampler = img->createTextureSampler(name, samplerInfo.magFilter, samplerInfo.minFilter, samplerInfo.addressMode, samplerInfo.anisotropy);
	descriptor_->updateDescriptorSets({ makeDescriptorWrite(sampler->getImageInfo(), arrayIdx, 1) });
	textureSamplersDI_[name] = std::make_pair(sampler, arrayIdx);
	return arrayIdx;
}

Material* TextureManager::requestMaterial(const RendererTypes type, const std::string name)
{
	if (!materials_[name]) {
		Material* mat = new Material();
		Materials::loadMaterial(this, type, name, &materialData_[materialCount_]);
		mat->data = &materialData_[materialCount_];
		mat->size = Materials::materialSizeLUT[(size_t)type];

		materialCount_ += mat->size;
		materials_[name] = mat;
	}
	return materials_[name];
}

VkWriteDescriptorSet TextureManager::makeDescriptorWrite(VkDescriptorImageInfo imageInfo, uint32_t idx, uint32_t count)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = (uint32_t)GlobalRenderDataBindings::kTextureArray;
	write.dstArrayElement = idx;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = count;
	write.pBufferInfo = 0;
	write.dstSet = descriptor_->getSet(descriptorSetIdx_);
	write.pImageInfo = &imageInfo;
	return write;
}

Image* TextureManager::allocateImage(std::string name, VkImageCreateInfo createInfo, MemoryAllocationPattern allocationPattern, ImageParameters parameters)
{
	return textures_.find(name) != textures_.end() ? nullptr : new Image(logicDevice_, createInfo, allocationPattern, parameters, name);
}
