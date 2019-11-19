// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "MemoryAllocation.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class PhysicalDevice;
		class Image;
		struct GraphicsSystemDetails;

		// Interface for https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/index.html
		// Uses the pImpl technique to avoid including vk_mem_alloc in lots of places.
		class DeviceMemory {
			friend class LogicDevice;
			class Impl;
		public:
			const MemoryAllocationDetails createBuffer(std::string debugName, MemoryAllocationPattern pattern, VkBufferUsageFlags bufferUsage, 
				VkDeviceSize size, MemoryAccessType accessType = MemoryAccessType::kDirect);
			const MemoryAllocationDetails createImage(MemoryAllocationPattern pattern, VkImageCreateInfo imageCreateInfo, std::string debugName);
			void deleteAllocation(AllocationID id, VkBuffer buffer);
			void deleteAllocation(AllocationID id, VkImage image);
			void* mapMemory(const AllocationID& id);
			void unmapMemory(const AllocationID& id);
			void transferMemory(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);
			void transferMemory(const VkBuffer& srcBuffer, const VkImage& dstImage, VkDeviceSize srcOffset, uint32_t width, uint32_t height, 
				VkShaderStageFlags stages = VK_SHADER_STAGE_FRAGMENT_BIT, Image* image = nullptr);
			void changeImageLayout(VkImageMemoryBarrier barrier, VkPipelineStageFlags oldStage, VkPipelineStageFlags newStage, VkCommandBuffer& cmdBuffer);
			void changeImageLayout(VkImageMemoryBarrier barrier, VkPipelineStageFlags oldStage, VkPipelineStageFlags newStage);

		private:
			DeviceMemory(PhysicalDevice* physicalDevice, LogicDevice* logicDevice, VkCommandBuffer transferCmdBuffer, VkQueue queue);
			~DeviceMemory();
			Impl* pImpl_;
		};
	}
}
