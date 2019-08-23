#include "Image.h"
#include "DeviceMemory.h"
#include "LogicDevice.h"
#include "TextureSampler.h"

using namespace QZL;
using namespace Graphics;

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

	if (imageParameters.newLayout != VK_IMAGE_LAYOUT_MAX_ENUM && imageParameters.oldLayout != imageParameters.newLayout) {
		changeLayout(imageParameters);
	}
}

Image::~Image()
{
	vkDestroyImageView(*logicDevice_, imageView_, nullptr);
	logicDevice_->getDeviceMemory()->deleteAllocation(imageDetails_.id, imageDetails_.image);
}

void Image::changeLayout(ImageParameters imageParameters)
{
	logicDevice_->getDeviceMemory()->changeImageLayout(imageDetails_.image, imageParameters.oldLayout, imageParameters.newLayout, format_, mipLevels_);
	layout_ = imageParameters.newLayout;
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
