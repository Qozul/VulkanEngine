#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class PhysicalDevice;
		class SwapChain;
		class Descriptor;
		struct GraphicsSystemDetails;

		enum class QueueFamilyType : size_t {
			kGraphicsQueue = 0,
			kPresentationQueue,
			kComputeQueue,
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

			static constexpr char const* kDescriptorRequirementsName = "../descriptor-requirements.txt";
		public:
			VkDevice getLogicDevice() const;
			VkPhysicalDevice getPhysicalDevice() const;

			DeviceMemory* getDeviceMemory() const;
			const uint32_t getFamilyIndex(QueueFamilyType type) const;
			const std::vector<uint32_t>& getAllIndices() const;
			VkQueue getQueueHandle(QueueFamilyType type) const;
			const bool supportsOptionalExtension(OptionalExtensions ext) const;

			operator VkDevice() const {
				return device_;
			}

			VkCommandBuffer getComputeCommandBuffer() const;
			Descriptor* getPrimaryDescriptor() const;

		private:
			LogicDevice(PhysicalDevice* physicalDevice, VkDevice device, const GraphicsSystemDetails& sysDetails, DeviceSurfaceCapabilities& surfaceCapabilities,
				std::vector<uint32_t> indices, std::vector<VkQueue> handles);
			~LogicDevice();
			void createCommandPools(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags, VkCommandPool* commandPool);
			void createCommandBuffers(std::vector<VkCommandBuffer>& buffers, VkCommandPool pool, uint32_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			void readDescriptorRequirements(std::vector<std::pair<VkDescriptorType, uint32_t>>& descriptorTypes, uint32_t& setCount);
			void createPrimaryDescriptor();

			VkDevice device_;
			VkCommandPool primaryCommandPool_;
			VkCommandPool computeCommandPool_;
			std::vector<VkCommandBuffer> commandBuffers_;
			std::vector<VkCommandBuffer> computeCommandBuffers_;

			PhysicalDevice* physicalDevice_; // Hold physical device so only logic device needs to be passed around
			DeviceMemory* deviceMemory_;

			std::vector<uint32_t> queueFamilyIndices_;
			std::vector<VkQueue> queueHandles_;

			Descriptor* primaryDescriptor_;
		};
	}
}
