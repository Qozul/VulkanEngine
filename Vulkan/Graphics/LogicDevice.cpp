#include "LogicDevice.h"
#include "SwapChain.h"
#include "GraphicsMaster.h"
#include "PhysicalDevice.h"
#include "DeviceMemory.h"
#include "Descriptor.h"
using namespace QZL;
using namespace QZL::Graphics;

VkCommandBuffer LogicDevice::getComputeCommandBuffer() const
{
	ASSERT(computeCommandBuffers_.size() > 0);
	return computeCommandBuffers_[0];
}

Descriptor* LogicDevice::getPrimaryDescriptor() const
{
	return primaryDescriptor_;
}

LogicDevice::LogicDevice(PhysicalDevice* physicalDevice, VkDevice device, const GraphicsSystemDetails& sysDetails, DeviceSurfaceCapabilities& surfaceCapabilities,
	std::vector<uint32_t> indices, std::vector<VkQueue> handles)
	: physicalDevice_(physicalDevice), device_(device), queueFamilyIndices_(indices), queueHandles_(handles)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices_[static_cast<size_t>(QueueFamilyType::kGraphicsQueue)];

	CHECK_VKRESULT(vkCreateCommandPool(device_, &poolInfo, nullptr, &primaryCommandPool_)); 
	
	poolInfo.queueFamilyIndex = queueFamilyIndices_[static_cast<size_t>(QueueFamilyType::kComputeQueue)];

	CHECK_VKRESULT(vkCreateCommandPool(device_, &poolInfo, nullptr, &computeCommandPool_));

	createCommandBuffers(commandBuffers_, primaryCommandPool_, 4);
	createCommandBuffers(computeCommandBuffers_, computeCommandPool_, 1);

	std::vector<std::pair<VkDescriptorType, uint32_t>> descriptorTypes = {
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 14 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8 }
	};
	primaryDescriptor_ = new Descriptor(this, (uint32_t)6, descriptorTypes);

	// Need device memory before swap chain
	deviceMemory_ = new DeviceMemory(physicalDevice, this, commandBuffers_[0], getQueueHandle(QueueFamilyType::kGraphicsQueue)); // TODO change to transfer queue
	swapChain_ = new SwapChain(sysDetails.master, sysDetails.window, sysDetails.surface, this, surfaceCapabilities);
	swapChain_->setCommandBuffers(std::vector<VkCommandBuffer>(commandBuffers_.begin() + 1, commandBuffers_.end()));
}

LogicDevice::~LogicDevice()
{
	SAFE_DELETE(swapChain_);
	SAFE_DELETE(deviceMemory_);
	vkFreeCommandBuffers(device_, primaryCommandPool_, static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
	vkDestroyCommandPool(device_, primaryCommandPool_, nullptr);
	vkFreeCommandBuffers(device_, computeCommandPool_, static_cast<uint32_t>(computeCommandBuffers_.size()), computeCommandBuffers_.data());
	vkDestroyCommandPool(device_, computeCommandPool_, nullptr);
	vkDestroyDevice(device_, nullptr);
}

void LogicDevice::createCommandBuffers(std::vector<VkCommandBuffer>& buffers, VkCommandPool pool, uint32_t count, VkCommandBufferLevel level)
{
	// One for drawing each frame plus 1 for buffer/image transfer commands
	size_t startSize = buffers.size();
	buffers.resize(startSize + count);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = pool;
	allocInfo.level = level;
	allocInfo.commandBufferCount = count;

	CHECK_VKRESULT(vkAllocateCommandBuffers(device_, &allocInfo, &buffers[startSize]));
}

VkDevice LogicDevice::getLogicDevice() const
{
	return device_;
}

VkPhysicalDevice LogicDevice::getPhysicalDevice() const
{
	return physicalDevice_->getPhysicalDevice();
}

DeviceMemory* LogicDevice::getDeviceMemory() const
{
	return deviceMemory_;
}

const uint32_t LogicDevice::getFamilyIndex(QueueFamilyType type) const
{
	EXPECTS(type != QueueFamilyType::kNumQueueFamilyTypes);
	return queueFamilyIndices_[static_cast<unsigned int>(type)];
}

const std::vector<uint32_t>& LogicDevice::getAllIndices() const
{
	return queueFamilyIndices_;
}

VkQueue LogicDevice::getQueueHandle(QueueFamilyType type) const
{
	EXPECTS(type != QueueFamilyType::kNumQueueFamilyTypes);
	return queueHandles_[static_cast<size_t>(type)];
}

const bool LogicDevice::supportsOptionalExtension(OptionalExtensions ext)
{
	return physicalDevice_->optionalExtensionsEnabled_[ext];
}
