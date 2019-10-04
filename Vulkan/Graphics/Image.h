#pragma once
#include "MemoryAllocation.h"

namespace QZL {
	namespace Graphics {

		struct ImageParameters {
			VkImageViewType type;
			VkImageAspectFlagBits aspectBits;
			VkImageLayout newLayout;
		};

		class LogicDevice;
		class TextureSampler;
		class Image {
		public: 
			Image(const LogicDevice* logicDevice, const VkImageCreateInfo createInfo, MemoryAllocationPattern pattern, ImageParameters imageParameters);
			~Image();

			void changeLayout(ImageParameters imageParameters);
			void changeLayout(VkImageLayout newLayout);
			void changeLayout(VkImageLayout newLayout, VkCommandBuffer& cmdBuffer);

			const VkImageView& getImageView();
			const VkImage& getImage();
			const VkImageLayout& getLayout();
			VkDescriptorImageInfo getImageInfo() {
				return imageInfo_;
			}
			TextureSampler* createTextureSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float anisotropy);

			static VkImageMemoryBarrier makeMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout,
				uint32_t srcQueueIndex, uint32_t dstQueueIndex, VkImage img, VkImageSubresourceRange subresourceRange);
			
			static VkImageCreateInfo makeCreateInfo(VkImageType type, uint32_t mipLevels, uint32_t arrayLayers, VkFormat format,
				VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits samples, uint32_t width, uint32_t height, uint32_t depth = 1) {
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
			static VkDescriptorSetLayoutBinding makeBinding(uint32_t b, VkShaderStageFlags flags, VkSampler* immutableSampler = nullptr) {
				VkDescriptorSetLayoutBinding binding = {};
				binding.binding = b;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				binding.pImmutableSamplers = immutableSampler;
				binding.stageFlags = flags;
				return binding;
			}
			VkWriteDescriptorSet descriptorWrite(VkDescriptorSet set, uint32_t binding);
		private:
			MemoryAllocationDetails imageDetails_;
			VkImageView imageView_;
			VkFormat format_;
			VkImageLayout layout_;
			uint32_t mipLevels_;
			VkDescriptorImageInfo imageInfo_;
			const LogicDevice* logicDevice_;
		};
	}
}
