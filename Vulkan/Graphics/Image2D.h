#pragma once
#include "MemoryAllocation.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class LogicDevice;

		struct ImageParameters {
			VkImageAspectFlagBits aspectBits;
			VkImageLayout oldLayout;
			VkImageLayout newLayout;
		};

		class Image2D {
		public:
			Image2D(const LogicDevice* logicDevice, DeviceMemory* deviceMemory, const VkImageCreateInfo createInfo, MemoryAllocationPattern pattern,
				ImageParameters imageParameters);
			~Image2D();
			void changeLayout(ImageParameters imageParameters);
			static VkImageCreateInfo makeImageCreateInfo(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
				VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage);

			const VkImageView& getImageView();
			const VkImage& getImage();

		private:
			MemoryAllocationDetails imageDetails_;
			VkImageView imageView_;
			VkFormat format_;
			uint32_t mipLevels_;

			DeviceMemory* deviceMemory_;
			const LogicDevice* logicDevice_;
		};
	}
}
