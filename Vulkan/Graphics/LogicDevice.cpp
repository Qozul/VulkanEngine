#include "LogicDevice.h"
#include "SwapChain.h"
#include "GraphicsMaster.h"
#include "PhysicalDevice.h"
#include "DeviceMemory.h"
#include "Descriptor.h"
#include <fstream>
#include <sstream>

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
	createCommandPools(queueFamilyIndices_[static_cast<size_t>(QueueFamilyType::kGraphicsQueue)], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &primaryCommandPool_);
	createCommandPools(queueFamilyIndices_[static_cast<size_t>(QueueFamilyType::kComputeQueue)], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &computeCommandPool_);

	createCommandBuffers(commandBuffers_, primaryCommandPool_, 4);
	createCommandBuffers(computeCommandBuffers_, computeCommandPool_, 1);

	createPrimaryDescriptor();

	deviceMemory_ = new DeviceMemory(physicalDevice, this, commandBuffers_[0], getQueueHandle(QueueFamilyType::kGraphicsQueue)); // TODO change to transfer queue
}

LogicDevice::~LogicDevice()
{
	SAFE_DELETE(primaryDescriptor_);
	SAFE_DELETE(deviceMemory_);
	vkFreeCommandBuffers(device_, primaryCommandPool_, static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
	vkDestroyCommandPool(device_, primaryCommandPool_, nullptr);
	vkFreeCommandBuffers(device_, computeCommandPool_, static_cast<uint32_t>(computeCommandBuffers_.size()), computeCommandBuffers_.data());
	vkDestroyCommandPool(device_, computeCommandPool_, nullptr);
	vkDestroyDevice(device_, nullptr);
}

void LogicDevice::createCommandPools(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags, VkCommandPool* commandPool)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = flags;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	CHECK_VKRESULT(vkCreateCommandPool(device_, &poolInfo, nullptr, commandPool));
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

void LogicDevice::readDescriptorRequirements(std::vector<std::pair<VkDescriptorType, uint32_t>>& descriptorTypes, uint32_t& setCount)
{
	std::ifstream requirementsFile(kDescriptorRequirementsName);
	ASSERT(requirementsFile.is_open());
	std::string line;
	while (std::getline(requirementsFile, line)) {
		// Ignore comments
		if (line != "" && line.at(0) != '#') {
			std::istringstream iss(line);
			int descriptorType, count, sCount;
			ASSERT(iss >> descriptorType >> count >> sCount);
			setCount += sCount;
			descriptorTypes.emplace_back(static_cast<VkDescriptorType>(descriptorType), static_cast<uint32_t>(count));
		}
	}
	requirementsFile.close();
}

void LogicDevice::createPrimaryDescriptor()
{
	std::vector<std::pair<VkDescriptorType, uint32_t>> descriptorTypes;
	uint32_t setCount = 0;
	readDescriptorRequirements(descriptorTypes, setCount);

	primaryDescriptor_ = new Descriptor(this, setCount, descriptorTypes);
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

const bool LogicDevice::supportsOptionalExtension(OptionalExtensions ext) const
{
	return physicalDevice_->optionalExtensionsEnabled_.at(ext);
}
