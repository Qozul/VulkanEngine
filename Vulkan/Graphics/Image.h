// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "MemoryAllocation.h"

namespace QZL {
	namespace Graphics {

		struct ImageParameters {
			VkImageViewType type;
			VkImageAspectFlagBits aspectBits;
			VkImageLayout newLayout;
			ImageParameters(VkImageViewType vt, VkImageAspectFlagBits ab, VkImageLayout l) : type(vt), aspectBits(ab), newLayout(l) { }
		};

		class LogicDevice;
		class TextureSampler;
		class Image {
		public: 
			Image(const LogicDevice* logicDevice, const VkImageCreateInfo createInfo, MemoryAllocationPattern pattern, ImageParameters imageParameters, std::string debugName = "");
			~Image();

			void changeLayout(VkImageLayout newLayout, VkPipelineStageFlags oldStageFlags = (VkPipelineStageFlags)0, VkPipelineStageFlags newStageFlags = (VkPipelineStageFlags)0, 
				VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
			void changeLayout(VkCommandBuffer& cmdBuffer, VkImageLayout newLayout, VkPipelineStageFlags oldStageFlags = (VkPipelineStageFlags)0, 
				VkPipelineStageFlags newStageFlags = (VkPipelineStageFlags)0, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

			const VkImageView& getImageView();
			const VkImage& getImage();
			const VkImageLayout& getLayout();
			VkDescriptorImageInfo& getImageInfo() {
				return imageInfo_;
			}
			TextureSampler* createTextureSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float anisotropy);

			static VkAccessFlags imageLayoutToAccessFlags(VkImageLayout layout);
			static VkPipelineStageFlags imageLayoutToStage(VkImageLayout layout);
			static VkImageAspectFlags imageLayoutToAspectMask(VkImageLayout layout, VkFormat format);
			static VkImageCreateInfo makeCreateInfo(VkImageType type, uint32_t mipLevels, uint32_t arrayLayers, VkFormat format,
				VkImageTiling tiling, VkImageUsageFlags usage, VkSampleCountFlagBits samples, uint32_t width, uint32_t height, uint32_t depth = 1);

			static VkDescriptorSetLayoutBinding makeBinding(uint32_t b, VkShaderStageFlags flags, VkSampler* immutableSampler = nullptr);
			VkImageMemoryBarrier makeImageMemoryBarrier(const VkImageLayout newLayout, VkImageAspectFlags aspectMask);
			VkWriteDescriptorSet descriptorWrite(VkDescriptorSet set, uint32_t binding);
		private:
			MemoryAllocationDetails imageDetails_;
			VkImageView imageView_;
			VkFormat format_;
			uint32_t mipLevels_;
			VkDescriptorImageInfo imageInfo_;
			const LogicDevice* logicDevice_;
		};
	}
}
