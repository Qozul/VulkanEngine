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

			void changeLayout(VkImageLayout newLayout, VkPipelineStageFlags oldStageFlags = (VkPipelineStageFlags)0, VkPipelineStageFlags newStageFlags = (VkPipelineStageFlags)0);
			void changeLayout(VkCommandBuffer& cmdBuffer, VkImageLayout newLayout, VkPipelineStageFlags oldStageFlags = (VkPipelineStageFlags)0, 
				VkPipelineStageFlags newStageFlags = (VkPipelineStageFlags)0);

			const VkImageView& getImageView();
			const VkImage& getImage();
			const VkImageLayout& getLayout();
			VkDescriptorImageInfo& getImageInfo() {
				return imageInfo_;
			}
			TextureSampler* createTextureSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float anisotropy);

			static VkAccessFlags imageLayoutToAccessFlags(VkImageLayout layout) {
				switch (layout) {
				case VK_IMAGE_LAYOUT_UNDEFINED:
					return static_cast<VkAccessFlags>(0);
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

			static VkPipelineStageFlags imageLayoutToStage(VkImageLayout layout) {
				// Note the lack of shader read only optimal because it could come from any stage to any stage
				switch (layout) {
				case VK_IMAGE_LAYOUT_UNDEFINED:
					return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					return VK_PIPELINE_STAGE_TRANSFER_BIT;
				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				case VK_IMAGE_LAYOUT_GENERAL:
					return VK_SHADER_STAGE_COMPUTE_BIT;
				default:
					ASSERT(false);
				}
			}

			static VkImageAspectFlags imageLayoutToAspectMask(VkImageLayout layout, VkFormat format) {
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
			VkImageMemoryBarrier makeImageMemoryBarrier(const VkImageLayout newLayout, VkImageAspectFlags aspectMask) {
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
