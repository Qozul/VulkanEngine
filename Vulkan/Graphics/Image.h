#pragma once
#include "MemoryAllocation.h"

namespace QZL {
	namespace Graphics {

		struct ImageParameters {
			VkImageViewType type;
			VkImageAspectFlagBits aspectBits;
			VkImageLayout oldLayout;
			VkImageLayout newLayout;
		};

		class LogicDevice;
		class TextureSampler;
		class Image {
		public: 
			Image(const LogicDevice* logicDevice, const VkImageCreateInfo createInfo, MemoryAllocationPattern pattern, ImageParameters imageParameters);
			~Image();

			void changeLayout(ImageParameters imageParameters);

			const VkImageView& getImageView();
			const VkImage& getImage();
			const VkImageLayout& getLayout();
			TextureSampler* createTextureSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float anisotropy);

			static VkImageCreateInfo makeCreateInfo(VkImageType type, uint32_t mipLevels, uint32_t arrayLayers, VkFormat format,
				VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits samples, VkImageLayout initialLayout, uint32_t width, uint32_t height, uint32_t depth = 1) {
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
				createInfo.initialLayout = initialLayout;
				createInfo.usage = usage;
				createInfo.samples = samples;
				createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				return createInfo;
			}
		private:
			MemoryAllocationDetails imageDetails_;
			VkImageView imageView_;
			VkFormat format_;
			VkImageLayout layout_;
			uint32_t mipLevels_;
			const LogicDevice* logicDevice_;
		};
	}
}
