// Author: Ralph Ridley
// Date: 01/11/19
#include "DeviceMemory.h"
#include "GraphicsMaster.h"
#include "PhysicalDevice.h"
#include "LogicDevice.h"
#include "Validation.h"
#include "Image.h"
#include "vk_mem_alloc.h"

using namespace QZL;
using namespace QZL::Graphics;

class DeviceMemory::Impl {
	friend class DeviceMemory;
private:
	const MemoryAllocationDetails createBuffer(std::string debugName, MemoryAllocationPattern pattern, VkBufferUsageFlags bufferUsage, 
		VkDeviceSize size, MemoryAccessType accessType = MemoryAccessType::kDirect);
	const MemoryAllocationDetails createImage(MemoryAllocationPattern pattern, VkImageCreateInfo imageCreateInfo, std::string debugName);
	void deleteAllocation(AllocationID id, VkBuffer buffer);
	void deleteAllocation(AllocationID id, VkImage image);
	void* mapMemory(const AllocationID& id);
	void unmapMemory(const AllocationID& id);
	void transferMemory(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size);
	void transferMemory(const VkBuffer& srcBuffer, const VkImage& dstImage, VkDeviceSize srcOffset, uint32_t width, uint32_t height, VkShaderStageFlags stages, Image* image);
	void transferMemory(const VkBuffer& srcBuffer, const VkImage& dstImage, VkBufferImageCopy* copyRanges, uint32_t count);
	void changeImageLayout(VkImageMemoryBarrier barrier, VkPipelineStageFlags oldStage, VkPipelineStageFlags newStage, VkCommandBuffer& cmdBuffer);
	void changeImageLayout(VkImageMemoryBarrier barrier, VkPipelineStageFlags oldStage, VkPipelineStageFlags newStage);

	Impl(PhysicalDevice* physicalDevice, LogicDevice* logicDevice, VkCommandBuffer transferCmdBuffer, VkQueue queue);
	~Impl();

	// Ensure mapped access is possible if requested
	void fixAccessType(MemoryAccessType& access, VmaAllocationInfo allocInfo, VkMemoryPropertyFlags memFlags);
	// Choose VmaCreateInfo based on the selected pattern
	VmaAllocationCreateInfo makeVmaCreateInfo(MemoryAllocationPattern pattern, MemoryAccessType& access);

	void selectImageLayoutInfo(const VkImage& image, const VkImageLayout oldLayout, const VkImageLayout newLayout, const VkFormat& format, uint32_t mipLevels,
		VkPipelineStageFlags& oldStage, VkPipelineStageFlags& newStage, VkImageMemoryBarrier& barrier);

	VmaAllocator allocator_;
	AllocationID availableId_; // 0 reserved for invalid id
	std::map<AllocationID, VmaAllocation> allocations_;
	VkQueue queue_;
	VkCommandBuffer transferCmdBuffer_;
	LogicDevice* logicDevice_;
};

DeviceMemory::Impl::Impl(PhysicalDevice* physicalDevice, LogicDevice* logicDevice, VkCommandBuffer transferCmdBuffer, VkQueue queue)
	: availableId_(1), logicDevice_(logicDevice), transferCmdBuffer_(transferCmdBuffer), queue_(queue)
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice->getPhysicalDevice();
	allocatorInfo.device = *logicDevice;

	vmaCreateAllocator(&allocatorInfo, &allocator_);
}

DeviceMemory::Impl::~Impl()
{
	if (!allocations_.empty()) {
		for (auto allocation : allocations_) {
			DEBUG_LOG("VMA Allocation not deleted, id: " << allocation.first);
		}
	}

	vmaDestroyAllocator(allocator_);
}

const MemoryAllocationDetails DeviceMemory::Impl::createBuffer(std::string debugName, MemoryAllocationPattern pattern, VkBufferUsageFlags bufferUsage, VkDeviceSize size, MemoryAccessType accessType)
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
	Validation::addDebugName(logicDevice_, VK_OBJECT_TYPE_BUFFER, (uint64_t)allocationDetails.buffer, debugName);

	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties(allocator_, allocInfo.memoryType, &memFlags);

	fixAccessType(allocationDetails.access, allocInfo, memFlags);

	return allocationDetails;
}

const MemoryAllocationDetails DeviceMemory::Impl::createImage(MemoryAllocationPattern pattern, VkImageCreateInfo imageCreateInfo, std::string debugName)
{
	MemoryAllocationDetails allocationDetails = {};
	allocationDetails.id = availableId_++;

	VmaAllocationCreateInfo allocCreateInfo = makeVmaCreateInfo(pattern, allocationDetails.access);
	VmaAllocationInfo allocInfo;

	CHECK_VKRESULT(vmaCreateImage(allocator_, &imageCreateInfo, &allocCreateInfo, &allocationDetails.image, &allocations_[allocationDetails.id], &allocInfo));
	Validation::addDebugName(logicDevice_, VK_OBJECT_TYPE_IMAGE, (uint64_t)allocationDetails.image, debugName);

	VkMemoryPropertyFlags memFlags;
	vmaGetMemoryTypeProperties(allocator_, allocInfo.memoryType, &memFlags);

	fixAccessType(allocationDetails.access, allocInfo, memFlags);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(*logicDevice_, allocationDetails.image, &memRequirements);
	allocationDetails.size = memRequirements.size;

	return allocationDetails;
}

void DeviceMemory::Impl::deleteAllocation(AllocationID id, VkBuffer buffer)
{
	vmaDestroyBuffer(allocator_, buffer, allocations_[id]);
	allocations_.erase(id);
}

void DeviceMemory::Impl::deleteAllocation(AllocationID id, VkImage image)
{
	vmaDestroyImage(allocator_, image, allocations_[id]);
	allocations_.erase(id);
}

void* DeviceMemory::Impl::mapMemory(const AllocationID& id)
{
	void* mappedData;
	CHECK_VKRESULT(vmaMapMemory(allocator_, allocations_[id], &mappedData));
	return mappedData;
}

void DeviceMemory::Impl::unmapMemory(const AllocationID& id)
{
	vmaUnmapMemory(allocator_, allocations_[id]);
}

void DeviceMemory::Impl::transferMemory(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
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

void DeviceMemory::Impl::transferMemory(const VkBuffer& srcBuffer, const VkImage& dstImage, VkDeviceSize srcOffset, uint32_t width, uint32_t height, VkShaderStageFlags stages, Image* image)
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

	if (image != nullptr) image->generateMipmaps(transferCmdBuffer_, stages);

	vkEndCommandBuffer(transferCmdBuffer_);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCmdBuffer_;

	vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue_);
}
void DeviceMemory::Impl::transferMemory(const VkBuffer& srcBuffer, const VkImage& dstImage, VkBufferImageCopy* copyRanges, uint32_t count)
{
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(transferCmdBuffer_, &beginInfo);

	vkCmdCopyBufferToImage(transferCmdBuffer_, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, count, copyRanges);

	vkEndCommandBuffer(transferCmdBuffer_);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCmdBuffer_;

	vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue_);
}

void DeviceMemory::Impl::changeImageLayout(VkImageMemoryBarrier barrier, VkPipelineStageFlags oldStage, VkPipelineStageFlags newStage, VkCommandBuffer& cmdBuffer)
{
	vkCmdPipelineBarrier(cmdBuffer, oldStage, newStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void DeviceMemory::Impl::changeImageLayout(VkImageMemoryBarrier barrier, VkPipelineStageFlags oldStage, VkPipelineStageFlags newStage)
{
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(transferCmdBuffer_, &beginInfo);

	changeImageLayout(barrier, oldStage, newStage, transferCmdBuffer_);

	vkEndCommandBuffer(transferCmdBuffer_);

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCmdBuffer_;

	vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue_);
}

void DeviceMemory::Impl::fixAccessType(MemoryAccessType& access, VmaAllocationInfo allocInfo, VkMemoryPropertyFlags memFlags)
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

VmaAllocationCreateInfo DeviceMemory::Impl::makeVmaCreateInfo(MemoryAllocationPattern pattern, MemoryAccessType& access)
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

void DeviceMemory::Impl::selectImageLayoutInfo(const VkImage& image, const VkImageLayout oldLayout, const VkImageLayout newLayout, const VkFormat& format, uint32_t mipLevels,
	VkPipelineStageFlags& oldStage, VkPipelineStageFlags& newStage, VkImageMemoryBarrier& barrier)
{
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
	barrier.subresourceRange.aspectMask = 0;

	switch (oldLayout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		barrier.srcAccessMask = 0;
		oldStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		oldStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		oldStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		oldStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		oldStage = VK_SHADER_STAGE_FRAGMENT_BIT;
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	case VK_IMAGE_LAYOUT_GENERAL:
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		oldStage = VK_SHADER_STAGE_COMPUTE_BIT;
		break;
	default:
		ASSERT(false); // Old layout invalid
	}

	switch (newLayout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		newStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		newStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		newStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		newStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	case VK_IMAGE_LAYOUT_GENERAL:
		// For the moment, image layout general is only used for storage images in compute stages.
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		newStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	default:
		ASSERT(false); // New layout invalid.
	}
}

// pImple interface comes below

DeviceMemory::DeviceMemory(PhysicalDevice* physicalDevice, LogicDevice* logicDevice, VkCommandBuffer transferCmdBuffer, VkQueue queue)
	: pImpl_(new DeviceMemory::Impl(physicalDevice, logicDevice, transferCmdBuffer, queue))
{
}
DeviceMemory::~DeviceMemory()
{
	delete pImpl_;
}
const MemoryAllocationDetails DeviceMemory::createBuffer(std::string debugName, MemoryAllocationPattern pattern, VkBufferUsageFlags bufferUsage,
	VkDeviceSize size, MemoryAccessType accessType)
{
	return pImpl_->createBuffer(debugName, pattern, bufferUsage, size, accessType);
}
const MemoryAllocationDetails DeviceMemory::createImage(MemoryAllocationPattern pattern, VkImageCreateInfo imageCreateInfo, std::string debugName)
{
	return pImpl_->createImage(pattern, imageCreateInfo, debugName);
}
void DeviceMemory::deleteAllocation(AllocationID id, VkBuffer buffer)
{
	pImpl_->deleteAllocation(id, buffer);
}
void DeviceMemory::deleteAllocation(AllocationID id, VkImage image)
{
	pImpl_->deleteAllocation(id, image);
}
void* DeviceMemory::mapMemory(const AllocationID& id)
{
	return pImpl_->mapMemory(id);
}
void DeviceMemory::unmapMemory(const AllocationID& id)
{
	pImpl_->unmapMemory(id);
}
void DeviceMemory::transferMemory(const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size)
{
	pImpl_->transferMemory(srcBuffer, dstBuffer, srcOffset, dstOffset, size);
}
void DeviceMemory::transferMemory(const VkBuffer& srcBuffer, const VkImage& dstImage, VkDeviceSize srcOffset, uint32_t width, uint32_t height, VkShaderStageFlags stages, Image* image)
{
	pImpl_->transferMemory(srcBuffer, dstImage, srcOffset, width, height, stages, image);
}
void DeviceMemory::transferMemory(const VkBuffer& srcBuffer, const VkImage& dstImage, VkBufferImageCopy* copyRanges, uint32_t count)
{
	pImpl_->transferMemory(srcBuffer, dstImage, copyRanges, count);
}
void DeviceMemory::changeImageLayout(VkImageMemoryBarrier barrier, VkPipelineStageFlags oldStage, VkPipelineStageFlags newStage, VkCommandBuffer& cmdBuffer)
{
	pImpl_->changeImageLayout(barrier, oldStage, newStage, cmdBuffer);
}
void DeviceMemory::changeImageLayout(VkImageMemoryBarrier barrier, VkPipelineStageFlags oldStage, VkPipelineStageFlags newStage)
{
	pImpl_->changeImageLayout(barrier, oldStage, newStage);
}
