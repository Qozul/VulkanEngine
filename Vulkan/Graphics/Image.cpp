#include "Image.h"
#include "DeviceMemory.h"
#include "LogicDevice.h"
#include "TextureSampler.h"

using namespace QZL;
using namespace Graphics;

Image::Image(const LogicDevice* logicDevice, const VkImageCreateInfo createInfo, MemoryAllocationPattern pattern, ImageParameters imageParameters)
	: logicDevice_(logicDevice), format_(createInfo.format), mipLevels_(createInfo.mipLevels)
{
	imageInfo_ = {};
	imageDetails_ = logicDevice->getDeviceMemory()->createImage(pattern, createInfo);

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = imageDetails_.image;
	viewInfo.viewType = imageParameters.type;
	viewInfo.format = createInfo.format;
	viewInfo.subresourceRange.aspectMask = imageParameters.aspectBits;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = createInfo.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = createInfo.arrayLayers;

	CHECK_VKRESULT(vkCreateImageView(*logicDevice, &viewInfo, nullptr, &imageView_));
	imageInfo_.imageView = imageView_;

	changeLayout(imageParameters.newLayout);

}

Image::~Image()
{
	vkDestroyImageView(*logicDevice_, imageView_, nullptr);
	logicDevice_->getDeviceMemory()->deleteAllocation(imageDetails_.id, imageDetails_.image);
}

void Image::changeLayout(VkImageLayout newLayout, VkPipelineStageFlags oldStageFlags, VkPipelineStageFlags newStageFlags) {
	VkImageAspectFlags aspectMask = imageInfo_.imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? imageLayoutToAspectMask(imageInfo_.imageLayout, format_) :
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? imageLayoutToAspectMask(newLayout, format_) : VK_IMAGE_ASPECT_COLOR_BIT;
	auto barrier = makeImageMemoryBarrier(newLayout, aspectMask);
	VkPipelineStageFlags oldStage = oldStageFlags == 0 ? imageLayoutToStage(imageInfo_.imageLayout) : oldStageFlags;
	VkPipelineStageFlags newStage = newStageFlags == 0 ? imageLayoutToStage(newLayout) : newStageFlags;

	logicDevice_->getDeviceMemory()->changeImageLayout(barrier, oldStage, newStage);
	imageInfo_.imageLayout = newLayout;
}

void Image::changeLayout(VkCommandBuffer& cmdBuffer, VkImageLayout newLayout, VkPipelineStageFlags oldStageFlags, VkPipelineStageFlags newStageFlags) {

	VkImageAspectFlags aspectMask = imageInfo_.imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? imageLayoutToAspectMask(imageInfo_.imageLayout, format_) :
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ? imageLayoutToAspectMask(newLayout, format_) : VK_IMAGE_ASPECT_COLOR_BIT;
	auto barrier = makeImageMemoryBarrier(newLayout, aspectMask);
	VkPipelineStageFlags oldStage = oldStageFlags == 0 ? imageLayoutToStage(imageInfo_.imageLayout) : oldStageFlags;
	VkPipelineStageFlags newStage = newStageFlags == 0 ? imageLayoutToStage(newLayout) : newStageFlags;

	logicDevice_->getDeviceMemory()->changeImageLayout(barrier, oldStage, newStage, cmdBuffer);
	imageInfo_.imageLayout = newLayout;
}

const VkImageView& Image::getImageView()
{
	return imageView_;
}

const VkImage& Image::getImage()
{
	return imageDetails_.image;
}

const VkImageLayout& Image::getLayout()
{
	return imageInfo_.imageLayout;
}

TextureSampler* Image::createTextureSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float anisotropy)
{
	return new TextureSampler(logicDevice_, name, this, magFilter, minFilter, addressMode, anisotropy);
}

VkWriteDescriptorSet Image::descriptorWrite(VkDescriptorSet set, uint32_t binding)
{
	ASSERT(imageInfo_.imageLayout == VK_IMAGE_LAYOUT_GENERAL);
	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = set;
	descriptorWrite.dstBinding = binding;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo_;

	return descriptorWrite;
}
