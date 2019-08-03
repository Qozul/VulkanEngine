#pragma once
#include "VkUtil.h"
#include "OptionalExtentions.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class PhysicalDevice;
		class SwapChain;
		struct GraphicsSystemDetails;

		enum class QueueFamilyType : size_t {
			kGraphicsQueue = 0,
			kPresentationQueue,
			//kComputeQueue,
			//kTransferQueue,
			kNumQueueFamilyTypes // Do not index with this, this is the size
		};

		struct DeviceSurfaceCapabilities {
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		class LogicDevice {
			friend class PhysicalDevice;
			friend class GraphicsMaster;
		public:
			VkDevice getLogicDevice() const;
			VkPhysicalDevice getPhysicalDevice() const;

			DeviceMemory* getDeviceMemory() const;
			const uint32_t getFamilyIndex(QueueFamilyType type) const;
			const std::vector<uint32_t>& getAllIndices() const;
			VkQueue getQueueHandle(QueueFamilyType type) const;
			const bool supportsOptionalExtension(OptionalExtensions ext);

			operator VkDevice() const {
				return device_;
			}

		private:
			LogicDevice(PhysicalDevice* physicalDevice, VkDevice device, const GraphicsSystemDetails& sysDetails, DeviceSurfaceCapabilities& surfaceCapabilities,
				std::vector<uint32_t> indices, std::vector<VkQueue> handles);
			~LogicDevice();
			void createCommandBuffers();

			VkDevice device_;
			VkCommandPool primaryCommandPool_;
			std::vector<VkCommandBuffer> commandBuffers_;

			PhysicalDevice* physicalDevice_; // Hold physical device so only logic device needs to be passed around
			SwapChain* swapChain_;
			DeviceMemory* deviceMemory_;

			std::vector<uint32_t> queueFamilyIndices_;
			std::vector<VkQueue> queueHandles_;
		};
	}
}
