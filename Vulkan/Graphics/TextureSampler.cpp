#include "TextureSampler.h"
#include "LogicDevice.h"
#include "Image.h"

using namespace QZL;
using namespace QZL::Graphics;

TextureSampler::TextureSampler(const LogicDevice* logicDevice, const std::string& name, Image* texture, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode,
	float anisotropy)
	: logicDevice_(logicDevice), name_(name), texture_(texture), sampler_(VK_NULL_HANDLE)
{
	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = magFilter;
	createInfo.minFilter = minFilter;
	createInfo.addressModeU = addressMode;
	createInfo.addressModeV = addressMode;
	createInfo.addressModeW = addressMode;
	createInfo.anisotropyEnable = anisotropy > 0.0f ? VK_TRUE : VK_FALSE;
	createInfo.maxAnisotropy = anisotropy;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;
	CHECK_VKRESULT(vkCreateSampler(*logicDevice, &createInfo, nullptr, &sampler_));

	imageInfo_ = texture_->getImageInfo();
	imageInfo_.sampler = sampler_;
}

TextureSampler::~TextureSampler()
{
	vkDestroySampler(*logicDevice_, sampler_, nullptr);
}

VkWriteDescriptorSet TextureSampler::descriptorWrite(VkDescriptorSet set, uint32_t binding)
{
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = set;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo_;

	return descriptorWrite;
}

VkDescriptorImageInfo TextureSampler::getImageInfo()
{
	return imageInfo_;
}
