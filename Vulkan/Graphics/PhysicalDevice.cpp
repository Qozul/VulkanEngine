#include "PhysicalDevice.h"

#include "SwapChain.h"
#include "LogicDevice.h"
#include "GraphicsMaster.h"

using namespace QZL;
using namespace QZL::Graphics;

PhysicalDevice::PhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface)
	: device_(device)
{
	queueFamilyIndices_.resize(static_cast<unsigned int>(QueueFamilyType::kNumQueueFamilyTypes));
	queueHandles_.resize(static_cast<unsigned int>(QueueFamilyType::kNumQueueFamilyTypes));
	for (auto& index : queueFamilyIndices_)
		index = kInvalidIndex;
	ASSERT(findIndices(device, surface));

	deviceExtensions_.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

bool PhysicalDevice::isValid(DeviceSurfaceCapabilities& surfaceCapabilities, VkSurfaceKHR& surface)
{
	vkGetPhysicalDeviceProperties(device_, &properties_);
	vkGetPhysicalDeviceFeatures(device_, &features_);
	// Can expand this to see if features contains certain things if needed
	return properties_.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
		hasRequiredExtensions(surfaceCapabilities, surface) &&
		hasRequiredQueueFamilies() && features_.samplerAnisotropy;
}

LogicDevice* PhysicalDevice::createLogicDevice(const GraphicsSystemDetails& sysDetails, DeviceSurfaceCapabilities& surfaceCapabilities,
	uint32_t& enabledLayerCount, const char* const*& ppEnabledLayerNames)
{
	const float queuePriority = 1.0f;
	auto createInfos = getCreateQueueInfos(&queuePriority);

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = createInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(createInfos.size());
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledLayerCount = enabledLayerCount;
	deviceCreateInfo.ppEnabledLayerNames = ppEnabledLayerNames;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions_.data();

	if (optionalExtensionsEnabled_[OptionalExtensions::DESCRIPTOR_INDEXING]) {
		VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures = {};
		descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
		descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		deviceCreateInfo.pNext = &descriptorIndexingFeatures;
	}

	VkDevice logicDevice;
	CHECK_VKRESULT(vkCreateDevice(device_, &deviceCreateInfo, nullptr, &logicDevice));

	queueHandles_[static_cast<size_t>(QueueFamilyType::kGraphicsQueue)] = createQueueHandles(logicDevice, QueueFamilyType::kGraphicsQueue);
	queueHandles_[static_cast<size_t>(QueueFamilyType::kPresentationQueue)] = createQueueHandles(logicDevice, QueueFamilyType::kPresentationQueue);
	//queueHandles_[static_cast<size_t>(QueueFamilyType::kComputeQueue)] = createQueueHandles(logicDevice, QueueFamilyType::kComputeQueue);

	return new LogicDevice(this, logicDevice, sysDetails, surfaceCapabilities, queueFamilyIndices_, queueHandles_);
}

VkPhysicalDevice PhysicalDevice::getPhysicalDevice() const
{
	return device_;
}

bool PhysicalDevice::findIndices(VkPhysicalDevice& device, VkSurfaceKHR& surface)
{
	EXPECTS(device != VK_NULL_HANDLE && surface != VK_NULL_HANDLE);
	auto queueFamilies = obtainVkData<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, device);
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
			queueFamilyIndices_[static_cast<size_t>(QueueFamilyType::kGraphicsQueue)] = i;

		VkBool32 hasPresent = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &hasPresent);
		if (queueFamily.queueCount > 0 && hasPresent)
			queueFamilyIndices_[static_cast<size_t>(QueueFamilyType::kPresentationQueue)] = i;

		if (hasRequiredQueueFamilies())
			return true;
		else
			i++;
	}
	return false;
}

bool PhysicalDevice::hasRequiredQueueFamilies()
{
	for (const auto& index : queueFamilyIndices_) {
		if (index == kInvalidIndex)
			return false;
	}
	return true;
}

bool PhysicalDevice::hasRequiredExtensions(DeviceSurfaceCapabilities& surfaceCapabilities, VkSurfaceKHR& surface)
{
	auto availableExts = obtainVkData<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, device_, nullptr);
	auto test = false;
	for (auto& ext : availableExts) {
		// Optional extensions
		if (!strcmp(ext.extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)) {
			deviceExtensions_.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
			optionalExtensionsEnabled_[OptionalExtensions::DESCRIPTOR_INDEXING] = true;
		}
		else {
			optionalExtensionsEnabled_[OptionalExtensions::DESCRIPTOR_INDEXING] = false;
		}
		// Required
		if (!strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
			test = true;
			break;
		}
	}
	if (test) {
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_, surface, &surfaceCapabilities.capabilities);
		surfaceCapabilities.formats = obtainVkData<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, device_, surface);
		surfaceCapabilities.presentModes = obtainVkData<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, device_, surface);
		return !surfaceCapabilities.formats.empty() && !surfaceCapabilities.presentModes.empty();
	}
	return false;
}

std::vector<VkDeviceQueueCreateInfo> PhysicalDevice::getCreateQueueInfos(const float* queuePriority)
{
	EXPECTS(queuePriority != nullptr);
	std::vector<uint32_t> uniqueIndices = queueFamilyIndices_;
	uniqueIndices.erase(std::unique(uniqueIndices.begin(), uniqueIndices.end()), uniqueIndices.end());

	std::vector<VkDeviceQueueCreateInfo> createInfos;
	createInfos.reserve(uniqueIndices.size());

	for (const auto& index : uniqueIndices) {
		VkDeviceQueueCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		createInfo.queueFamilyIndex = index;
		createInfo.queueCount = 1;
		createInfo.pQueuePriorities = queuePriority;
		createInfos.push_back(createInfo);
	}
	return createInfos;
}

VkQueue PhysicalDevice::createQueueHandles(VkDevice logicDevice, QueueFamilyType type)
{
	EXPECTS(type != QueueFamilyType::kNumQueueFamilyTypes);
	VkQueue queue;
	uint32_t familyIndex = queueFamilyIndices_[static_cast<size_t>(type)];
	ASSERT(familyIndex != kInvalidIndex);
	vkGetDeviceQueue(logicDevice, familyIndex, 0, &queue);
	return queue;
}

VkQueue PhysicalDevice::getQueueHandle(QueueFamilyType type)
{
	EXPECTS(type != QueueFamilyType::kNumQueueFamilyTypes);
	return queueHandles_[static_cast<size_t>(type)];
}
