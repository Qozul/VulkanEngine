#include "Image2D.h"
#include "DeviceMemory.h"
#include "LogicDevice.h"

using namespace QZL;
using namespace QZL::Graphics;

Image2D::Image2D(const LogicDevice* logicDevice, DeviceMemory* deviceMemory, const VkImageCreateInfo createInfo, MemoryAllocationPattern pattern,
	ImageParameters imageParameters)
	: logicDevice_(logicDevice), deviceMemory_(deviceMemory), format_(createInfo.format), mipLevels_(createInfo.mipLevels)
{
	imageDetails_ = deviceMemory_->createImage(pattern, createInfo);

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = imageDetails_.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = createInfo.format;
	viewInfo.subresourceRange.aspectMask = imageParameters.aspectBits;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = createInfo.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	CHECK_VKRESULT(vkCreateImageView(*logicDevice, &viewInfo, nullptr, &imageView_));

	deviceMemory_->changeImageLayout(imageDetails_.image, imageParameters.oldLayout, imageParameters.newLayout, createInfo.format, createInfo.mipLevels);
}

Image2D::~Image2D()
{
	vkDestroyImageView(*logicDevice_, imageView_, nullptr);
	deviceMemory_->deleteAllocation(imageDetails_.id, imageDetails_.image);
}

void Image2D::changeLayout(ImageParameters imageParameters)
{
	deviceMemory_->changeImageLayout(imageDetails_.image, imageParameters.oldLayout, imageParameters.newLayout, format_, mipLevels_);
}

VkImageCreateInfo Image2D::makeImageCreateInfo(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, 
	VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage)
{
	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = 1;
	createInfo.mipLevels = mipLevels;
	createInfo.arrayLayers = 1;
	createInfo.format = format;
	createInfo.tiling = tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = usage;
	createInfo.samples = numSamples;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	return createInfo;
}

const VkImageView& Image2D::getImageView()
{
	return imageView_;
}

const VkImage& Image2D::getImage()
{
	return imageDetails_.image;
}
