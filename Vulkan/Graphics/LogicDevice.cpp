#include "LogicDevice.h"
#include "SwapChain.h"
#include "GraphicsMaster.h"
#include "PhysicalDevice.h"
#include "DeviceMemory.h"

using namespace QZL;
using namespace QZL::Graphics;

LogicDevice::LogicDevice(PhysicalDevice* physicalDevice, VkDevice device, const GraphicsSystemDetails& sysDetails, DeviceSurfaceCapabilities& surfaceCapabilities,
	std::vector<uint32_t> indices, std::vector<VkQueue> handles)
	: physicalDevice_(physicalDevice), device_(device), queueFamilyIndices_(indices), queueHandles_(handles)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices_[static_cast<size_t>(QueueFamilyType::kGraphicsQueue)];

	CHECK_VKRESULT(vkCreateCommandPool(device_, &poolInfo, nullptr, &primaryCommandPool_));

	createCommandBuffers();
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
	vkDestroyDevice(device_, nullptr);
}

void LogicDevice::createCommandBuffers()
{
	// One for drawing each frame plus 1 for buffer/image transfer commands
	commandBuffers_.resize(4);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = primaryCommandPool_;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

	CHECK_VKRESULT(vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()));
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
