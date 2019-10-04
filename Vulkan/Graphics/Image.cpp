#include "Image.h"
#include "DeviceMemory.h"
#include "LogicDevice.h"
#include "TextureSampler.h"

using namespace QZL;
using namespace Graphics;

// TODO VkImageViewType and "intended layout" rather than current
Image::Image(const LogicDevice* logicDevice, const VkImageCreateInfo createInfo, MemoryAllocationPattern pattern, ImageParameters imageParameters)
	: logicDevice_(logicDevice), format_(createInfo.format), mipLevels_(createInfo.mipLevels), layout_(createInfo.initialLayout)
{
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

	changeLayout(imageParameters);

	imageInfo_ = {};
	imageInfo_.imageLayout = layout_;
	imageInfo_.imageView = imageView_;
}

Image::~Image()
{
	vkDestroyImageView(*logicDevice_, imageView_, nullptr);
	logicDevice_->getDeviceMemory()->deleteAllocation(imageDetails_.id, imageDetails_.image);
}

void Image::changeLayout(ImageParameters imageParameters)
{
	changeLayout(imageParameters.newLayout);
}

void Image::changeLayout(VkImageLayout newLayout) {
	logicDevice_->getDeviceMemory()->changeImageLayout(imageDetails_.image, layout_, newLayout, format_, mipLevels_);
	layout_ = newLayout;
	imageInfo_.imageLayout = layout_;
}

void Image::changeLayout(VkImageLayout newLayout, VkCommandBuffer& cmdBuffer) {
	logicDevice_->getDeviceMemory()->changeImageLayout(imageDetails_.image, layout_, newLayout, format_, mipLevels_, cmdBuffer);
	layout_ = newLayout;
	imageInfo_.imageLayout = layout_;
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
	return layout_;
}

TextureSampler* Image::createTextureSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float anisotropy)
{
	return new TextureSampler(logicDevice_, name, this, magFilter, minFilter, addressMode, anisotropy);
}

VkImageMemoryBarrier Image::makeMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, 
	uint32_t srcQueueIndex, uint32_t dstQueueIndex, VkImage img, VkImageSubresourceRange subresourceRange)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = srcQueueIndex;
	barrier.dstQueueFamilyIndex = dstQueueIndex;
	barrier.image = img;
	barrier.subresourceRange = subresourceRange;
	return barrier;
}

VkWriteDescriptorSet Image::descriptorWrite(VkDescriptorSet set, uint32_t binding)
{
	ASSERT(layout_ == VK_IMAGE_LAYOUT_GENERAL);
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
