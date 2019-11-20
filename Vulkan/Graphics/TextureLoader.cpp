// Author: Ralph Ridley
// Date: 01/11/19
#include "TextureLoader.h"
#include "Image.h"
#include "LogicDevice.h"
#include "DeviceMemory.h"
#include "../../Shared/nv_dds.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../../Shared/stb_image.h"

using namespace QZL;
using namespace QZL::Graphics;

const std::string TextureLoader::kPath = "../Data/Textures/";
const std::string TextureLoader::kExt = ".dds";

TextureLoader::TextureLoader(const LogicDevice* logicDevice)
	: logicDevice_(logicDevice), deviceMemory_(logicDevice->getDeviceMemory())
{
}

TextureLoader::~TextureLoader()
{
}

unsigned char* TextureLoader::getCPUImage(std::string name, int width, int height, int channels, int format) 
{
	unsigned char* image = stbi_load((kPath + name).c_str(), &width, &height, &channels, format);
	return image;
}

void TextureLoader::freeCPUImage(unsigned char* image) 
{
	stbi_image_free(image);
}

// Adapted from https://vulkan-tutorial.com/Texture_mapping/Images
Image* TextureLoader::loadTexture(const std::string& fileName, VkShaderStageFlags stages)
{
	DEBUG_LOG("Loading texture " << fileName);
	nv_dds::CDDSImage image;
	image.load(kPath + fileName + kExt, false);
	ASSERT(image.is_valid());
	Image* texture = nullptr;
	if (!image.is_cubemap()) {
		VkFormat format = convertToVkFormat(image.get_format());
		MemoryAllocationDetails stagingBuffer = deviceMemory_->createBuffer("", MemoryAllocationPattern::kStaging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, image.get_size());
		uint8_t* data = static_cast<uint8_t*>(deviceMemory_->mapMemory(stagingBuffer.id));
		memcpy(data, image, image.get_size());
		deviceMemory_->unmapMemory(stagingBuffer.id);

		texture = new Image(logicDevice_, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 1, format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, image.get_width(), image.get_height()),
			MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL });

		deviceMemory_->transferMemory(stagingBuffer.buffer, texture->getImage(), 0, image.get_width(), image.get_height(), stages, texture);

		if (texture->getMipLevels() <= 1) {
			texture->changeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, stages);
		}

		deviceMemory_->deleteAllocation(stagingBuffer.id, stagingBuffer.buffer);
		image.clear();
	}
	else {
		VkFormat format = VK_FORMAT_BC3_UNORM_BLOCK;
		texture = new Image(logicDevice_, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 6, format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, image.get_width(), image.get_height(), 1, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT),
			MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL });

		MemoryAllocationDetails stagingBuffer = deviceMemory_->createBuffer("", MemoryAllocationPattern::kStaging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, image.get_size() * 6);
		
		std::vector<VkBufferImageCopy> bufferCopyRegions;
		uint32_t offset = 0;
		uint8_t* data = static_cast<uint8_t*>(deviceMemory_->mapMemory(stagingBuffer.id));
		for (size_t face = 0; face < 6; ++face) {
			memcpy(data + offset, image.get_cubemap_face(face), image.get_size());

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = image.get_width();
			bufferCopyRegion.imageExtent.height = image.get_height();
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;
			bufferCopyRegions.push_back(bufferCopyRegion);
			offset = image.get_size();
		}
		deviceMemory_->unmapMemory(stagingBuffer.id);

		deviceMemory_->transferMemory(stagingBuffer.buffer, texture->getImage(), bufferCopyRegions.data(), bufferCopyRegions.size());

		texture->changeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, stages);
		deviceMemory_->deleteAllocation(stagingBuffer.id, stagingBuffer.buffer);
		image.clear();
	}

	return texture;
}

Image* Graphics::TextureLoader::loadCubeTexture(const std::array<std::string, 6U> fileName, VkShaderStageFlags stages)
{
	nv_dds::CDDSImage image[6];
	for (size_t i = 0; i < 6; ++i) {
		DEBUG_LOG("Loading cubemap texture image " << i << fileName[i]);
		image[i].load(kPath + fileName[i] + kExt, false);
		ASSERT(image[i].is_valid());
	}
	Image* texture = nullptr;
	VkFormat format = convertToVkFormat(image[0].get_format());
	texture = new Image(logicDevice_, Image::makeCreateInfo(VK_IMAGE_TYPE_2D, 1, 6, format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, image[0].get_width(), image[0].get_height(), 1, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT),
		MemoryAllocationPattern::kStaticResource, { VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL });

	MemoryAllocationDetails stagingBuffer = deviceMemory_->createBuffer("", MemoryAllocationPattern::kStaging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, (size_t)image[0].get_size() * 6);

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	uint32_t offset = 0;
	uint8_t* data = static_cast<uint8_t*>(deviceMemory_->mapMemory(stagingBuffer.id));
	for (size_t face = 0; face < 6; ++face) {
		memcpy(data + offset, image[face], image[face].get_size());

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = face;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = image[0].get_width();
		bufferCopyRegion.imageExtent.height = image[0].get_height();
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = offset;
		bufferCopyRegions.push_back(bufferCopyRegion);
		offset = image[0].get_size();
	}
	deviceMemory_->unmapMemory(stagingBuffer.id);

	deviceMemory_->transferMemory(stagingBuffer.buffer, texture->getImage(), bufferCopyRegions.data(), bufferCopyRegions.size());

	texture->changeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, stages);
	deviceMemory_->deleteAllocation(stagingBuffer.id, stagingBuffer.buffer);
	for (size_t i = 0; i < 6; ++i) {
		image[i].clear();
	}
	return texture;
}

VkFormat TextureLoader::convertToVkFormat(unsigned int oldFormat)
{
	// TODO gamma correction, e.g. VK_FORMAT_BC1_RGB_SRGB_BLOCK
	switch (oldFormat) {
	case 33777:
		return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
	case 33778:
		return VK_FORMAT_BC2_UNORM_BLOCK;
	case 33779:
		return VK_FORMAT_BC3_UNORM_BLOCK;
	default:
		ASSERT(false);
	}
}
