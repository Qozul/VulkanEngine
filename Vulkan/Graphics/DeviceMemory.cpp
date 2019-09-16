#include "DeviceMemory.h"
#include "GraphicsMaster.h"
#include "PhysicalDevice.h"
#include "LogicDevice.h"

using namespace QZL;
using namespace QZL::Graphics;

DeviceMemory::DeviceMemory(PhysicalDevice* physicalDevice, LogicDevice* logicDevice, VkCommandBuffer transferCmdBuffer, VkQueue queue)
	: availableId_(1), logicDevice_(logicDevice), transferCmdBuffer_(transferCmdBuffer), queue_(queue)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice->getPhysicalDevice();
	allocatorInfo.device = *logicDevice;

	vmaCreateAllocator(&allocatorInfo, &allocator_);
}

DeviceMemory::~DeviceMemory()
{
	if (!allocations_.empty()) {
		for (auto allocation : allocations_) {
			DEBUG_LOG("VMA Allocation not deleted, id: " << allocation.first);
		}
	}

	vmaDestroyAllocator(allocator_);
}

const MemoryAllocationDetails DeviceMemory::createBuffer(MemoryAllocationPattern pattern, VkBufferUsageFlags bufferUsage, VkDeviceSize size, MemoryAccessType accessType)
{
	MemoryAllocationDetails allocationDetails = {};
	allocationDetails.size = size;
	allocationDetails.id = availableId_++;
	allocationDetails.access = accessType;

	VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = allocationDetails.size;
	bufferCreateInfo.usage = bufferUsage;
	
	VmaAllocationCreateInfo allocCreateInfo = makeVmaCreateInfo(pattern, allocationDetails.access);
	VmaAllocationInfo allocInfo;

	CHECK_VKRESULT(vmaCreateBuffer(allocator_, &bufferCreateInfo, &allocCreateInfo, &allocationDetails.buffer, &allocations_[allocationDetails.id], &allocInfo));

	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties(allocator_, allocInfo.memoryType, &memFlags);

	fixAccessType(allocationDetails.access, allocInfo, memFlags);

	return allocationDetails;
}

const MemoryAllocationDetails DeviceMemory::createImage(MemoryAllocationPattern pattern, VkImageCreateInfo imageCreateInfo)
{
	MemoryAllocationDetails allocationDetails = {};
	allocationDetails.id = availableId_++;

	VmaAllocationCreateInfo allocCreateInfo = makeVmaCreateInfo(pattern, allocationDetails.access);
	VmaAllocationInfo allocInfo;

	CHECK_VKRESULT(vmaCreateImage(allocator_, &imageCreateInfo, &allocCreateInfo, &allocationDetails.image, &allocations_[allocationDetails.id], &allocInfo));

	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties(allocator_, allocInfo.memoryType, &memFlags);

	fixAccessType(allocationDetails.access, allocInfo, memFlags);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(*logicDevice_, allocationDetails.image, &memRequirements);
	allocationDetails.size = memRequirements.size;

	return allocationDetails;
}

void DeviceMemory::deleteAllocation(AllocationID id, VkBuffer buffer)
{
	vmaDestroyBuffer(allocator_, buffer, allocations_[id]);
	allocations_.erase(id);
}

void DeviceMemory::deleteAllocation(AllocationID id, VkImage image)
{
	vmaDestroyImage(allocator_, image, allocations_[id]);
	allocations_.erase(id);
}

void* DeviceMemory::mapMemory(const AllocationID& id)
{
	void* mappedData;
	CHECK_VKRESULT(vmaMapMemory(allocator_, allocations_[id], &mappedData));
	return mappedData;
}

void DeviceMemory::unmapMemory(const AllocationID & id)
{
	vmaUnmapMemory(allocator_, allocations_[id]);
}

void DeviceMemory::transferMemory(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
{
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(transferCmdBuffer_, &beginInfo);

	VkBufferCopy copyRegion = { srcOffset, dstOffset, size };
	vkCmdCopyBuffer(transferCmdBuffer_, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(transferCmdBuffer_);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCmdBuffer_;

	vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue_);
}

void DeviceMemory::transferMemory(const VkBuffer& srcBuffer, const VkImage& dstImage, VkDeviceSize srcOffset, uint32_t width, uint32_t height)
{
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(transferCmdBuffer_, &beginInfo);

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = srcOffset;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(transferCmdBuffer_, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	vkEndCommandBuffer(transferCmdBuffer_);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCmdBuffer_;

	vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue_);
}

void DeviceMemory::changeImageLayout(const VkImage& image, const VkImageLayout oldLayout, const VkImageLayout newLayout, const VkFormat& format, uint32_t mipLevels)
{
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(transferCmdBuffer_, &beginInfo);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags oldStage;
	VkPipelineStageFlags newStage;

	switch (oldLayout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		barrier.srcAccessMask = 0;
		oldStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		oldStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	default:
		ASSERT(false); // Old layout invalid
	}

	switch (newLayout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		newStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		newStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		newStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		newStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	default:
		ASSERT(false); // New layout invalid.
	}

	vkCmdPipelineBarrier(transferCmdBuffer_, oldStage, newStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkEndCommandBuffer(transferCmdBuffer_);

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCmdBuffer_;

	vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue_);
}

void DeviceMemory::fixAccessType(MemoryAccessType& access, VmaAllocationInfo allocInfo, VkMemoryPropertyFlags memFlags)
{
	switch (access) {
	case MemoryAccessType::kPersistant:
		if (allocInfo.pUserData == nullptr)
			access = MemoryAccessType::kTransfer;
		break;
	case MemoryAccessType::kDirect:
		if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			access = MemoryAccessType::kTransfer;
		break;
	default:
		break;
	}
}

VmaAllocationCreateInfo DeviceMemory::makeVmaCreateInfo(MemoryAllocationPattern pattern, MemoryAccessType& access)
{
	VmaAllocationCreateInfo createInfo = {};
	switch (pattern) {
	case MemoryAllocationPattern::kRenderTarget:
		access = MemoryAccessType::kTransfer;
		createInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		break;
	case MemoryAllocationPattern::kStaticResource:
		access = MemoryAccessType::kTransfer;
		createInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		break;
	case MemoryAllocationPattern::kDynamicResource:
		access = access != MemoryAccessType::kPersistant ? MemoryAccessType::kDirect : access;
		createInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		break;
	case MemoryAllocationPattern::kReadback:
		access = access != MemoryAccessType::kPersistant ? MemoryAccessType::kDirect : access;
		createInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
		break;
	case MemoryAllocationPattern::kStaging:
		access = MemoryAccessType::kDirect;
		createInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		break;
	}

	switch (access) {
	case MemoryAccessType::kPersistant:
		createInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		break;
	case MemoryAccessType::kDirect:
		createInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		break;
	}

	return createInfo;
}
