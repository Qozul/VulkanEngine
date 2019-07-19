#include "TextureLoader.h"
#include "../../Shared/nv_dds.h"
#include "Image2D.h"
#include "LogicDevice.h"
#include "DeviceMemory.h"

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
	for (auto it : textures_) {
		SAFE_DELETE(it.second);
	}
}

// adapted from https://vulkan-tutorial.com/Texture_mapping/Images
Image2D* TextureLoader::loadTexture(const std::string& fileName)
{
	if (textures_.count(fileName))
		return textures_[fileName];

	DEBUG_LOG("Loading texture " << fileName);
	nv_dds::CDDSImage image;
	image.load(kPath + fileName + kExt, false);
	ASSERT(image.is_valid());
	VkFormat format = convertToVkFormat(image.get_format());

	MemoryAllocationDetails stagingBuffer = deviceMemory_->createBuffer(MemoryAllocationPattern::kStaging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, image.get_size());
	uint8_t* data = static_cast<uint8_t*>(deviceMemory_->mapMemory(stagingBuffer.id));
	memcpy(data, image, image.get_size());
	deviceMemory_->unmapMemory(stagingBuffer.id);

	// make Image2D and transfer the data from the staging buffer
	textures_[fileName] = new Image2D(logicDevice_, deviceMemory_, Image2D::makeImageCreateInfo(image.get_width(), image.get_height(), 1,
		VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT), MemoryAllocationPattern::kStaticResource,
		{ VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL });

	deviceMemory_->transferMemory(stagingBuffer.buffer, textures_[fileName]->getImage(), 0, image.get_width(), image.get_height());

	textures_[fileName]->changeLayout({ VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

	deviceMemory_->deleteAllocation(stagingBuffer.id, stagingBuffer.buffer);
	image.clear();

	return textures_[fileName];
}

VkFormat TextureLoader::convertToVkFormat(unsigned int oldFormat)
{
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
