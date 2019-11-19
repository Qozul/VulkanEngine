// Author: Ralph Ridley
// Date: 01/11/19
// Mipmapping reference https://vulkan-tutorial.com/Generating_Mipmaps for calculating mipmap levels and generating them
#include "Image.h"
#include "DeviceMemory.h"
#include "LogicDevice.h"
#include "TextureSampler.h"

using namespace QZL;
using namespace Graphics;

Image::Image(const LogicDevice* logicDevice, VkImageCreateInfo createInfo, MemoryAllocationPattern pattern, ImageParameters imageParameters, std::string debugName)
	: logicDevice_(logicDevice), format_(createInfo.format), width_(createInfo.extent.width), height_(createInfo.extent.height), mipLevels_(createInfo.mipLevels)
{
	if (createInfo.mipLevels > 1) {
		mipLevels_ = calculateMipLevels(createInfo.extent.width, createInfo.extent.height);
		createInfo.mipLevels = mipLevels_;
		createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	imageInfo_ = {};
	imageDetails_ = logicDevice->getDeviceMemory()->createImage(pattern, createInfo, debugName);

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

	changeLayout(imageParameters.newLayout, 0, 0, imageParameters.aspectBits);
}

Image::~Image()
{
	vkDestroyImageView(*logicDevice_, imageView_, nullptr);
	logicDevice_->getDeviceMemory()->deleteAllocation(imageDetails_.id, imageDetails_.image);
}

void Image::changeLayout(VkImageLayout newLayout, VkPipelineStageFlags oldStageFlags, VkPipelineStageFlags newStageFlags, VkImageAspectFlags aspectMask) 
{
	auto barrier = makeImageMemoryBarrier(newLayout, aspectMask);
	VkPipelineStageFlags oldStage = oldStageFlags == 0 ? imageLayoutToStage(imageInfo_.imageLayout) : oldStageFlags;
	VkPipelineStageFlags newStage = newStageFlags == 0 ? imageLayoutToStage(newLayout) : newStageFlags;

	logicDevice_->getDeviceMemory()->changeImageLayout(barrier, oldStage, newStage);
	imageInfo_.imageLayout = newLayout;
}

void Image::changeLayout(VkCommandBuffer& cmdBuffer, VkImageLayout newLayout, VkPipelineStageFlags oldStageFlags, VkPipelineStageFlags newStageFlags, 
	VkImageAspectFlags aspectMask) 
{
	auto barrier = makeImageMemoryBarrier(newLayout, aspectMask);
	VkPipelineStageFlags oldStage = oldStageFlags == 0 ? imageLayoutToStage(imageInfo_.imageLayout) : oldStageFlags;
	VkPipelineStageFlags newStage = newStageFlags == 0 ? imageLayoutToStage(newLayout) : newStageFlags;

	logicDevice_->getDeviceMemory()->changeImageLayout(barrier, oldStage, newStage, cmdBuffer);
	imageInfo_.imageLayout = newLayout;
}

uint32_t Image::calculateMipLevels(uint32_t width, uint32_t height)
{
	return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

void Image::generateMipmaps(VkCommandBuffer cmdBuffer, VkShaderStageFlags stages)
{
	if (mipLevels_ == 1) return;
	VkImageMemoryBarrier barrier = makeImageMemoryBarrier(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	int32_t mipWidth = width_;
	int32_t mipHeight = height_;
	for (uint32_t i = 1; i < mipLevels_; i++) {
		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmdBuffer, imageDetails_.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageDetails_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, stages, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels_ - 1;
	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, stages, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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

VkAccessFlags Image::imageLayoutToAccessFlags(VkImageLayout layout)
{
	switch (layout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		return static_cast<VkAccessFlags>(0);
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return VK_ACCESS_TRANSFER_READ_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return VK_ACCESS_TRANSFER_WRITE_BIT;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return VK_ACCESS_SHADER_READ_BIT;
	case VK_IMAGE_LAYOUT_GENERAL:
		return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	default:
		ASSERT(false);
	}
}

VkPipelineStageFlags Image::imageLayoutToStage(VkImageLayout layout)
{
	switch (layout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	case VK_IMAGE_LAYOUT_GENERAL:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	default:
		ASSERT(false);
	}
}

VkImageAspectFlags Image::imageLayoutToAspectMask(VkImageLayout layout, VkFormat format)
{
	if (layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		VkImageAspectFlags mask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
			mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return mask;
	}
	else {
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

VkImageCreateInfo Image::makeCreateInfo(VkImageType type, uint32_t mipLevels, uint32_t arrayLayers, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits samples, uint32_t width, uint32_t height, uint32_t depth)
{
	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = type;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = depth;
	createInfo.mipLevels = mipLevels;
	createInfo.arrayLayers = arrayLayers;
	createInfo.format = format;
	createInfo.tiling = tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = usage;
	createInfo.samples = samples;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	return createInfo;
}

VkDescriptorSetLayoutBinding Image::makeBinding(uint32_t b, VkShaderStageFlags flags, VkSampler* immutableSampler)
{
	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = b;
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	binding.pImmutableSamplers = immutableSampler;
	binding.stageFlags = flags;
	return binding;
}

VkImageMemoryBarrier Image::makeImageMemoryBarrier(const VkImageLayout newLayout, VkImageAspectFlags aspectMask)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = imageInfo_.imageLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = imageDetails_.image;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels_;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.aspectMask = aspectMask;

	barrier.srcAccessMask = imageLayoutToAccessFlags(imageInfo_.imageLayout);
	barrier.dstAccessMask = imageLayoutToAccessFlags(newLayout);

	return barrier;
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
