// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "LogicDevice.h"
#include "DeviceMemory.h"

namespace QZL
{
	namespace Graphics {
		class UniformBuffer;
		class StorageBuffer;
		class DynamicUniformBuffer;

		class DescriptorBuffer {
		public:
			virtual ~DescriptorBuffer();
			void init( MemoryAllocationPattern pattern, VkBufferUsageFlags flags, VkShaderStageFlags stageFlags);
			const VkDescriptorSetLayoutBinding& getBinding();
			VkWriteDescriptorSet descriptorWrite(VkDescriptorSet set);
			template<typename DataType>
			void uploadRange(DataType* data, VkDeviceSize size, VkDeviceSize offset);
			// Alternative to uploading a range directly, these allow the mapping to last longer
			// But if bind is called the caller must ensure unbind is also called
			void* bindRange();
			void unbindRange();
			const MemoryAllocationDetails& getBufferDetails() { return bufferDetails_; }

			template<typename T>
			static DescriptorBuffer* makeBuffer(const LogicDevice* logicDevice, MemoryAllocationPattern pattern, uint32_t binding, VkBufferUsageFlags flags, VkDeviceSize maxSize, VkShaderStageFlags stageFlags) {
				DescriptorBuffer* buf = new T(logicDevice, binding, maxSize);
				buf->init(pattern, flags, stageFlags);
				return buf;
			}
		protected:
			DescriptorBuffer(const LogicDevice* logicDevice, uint32_t binding, VkDeviceSize maxSize);
			virtual VkBufferUsageFlagBits getUsageBits() = 0;
			virtual VkDescriptorType getType() = 0;

			MemoryAllocationDetails bufferDetails_;
			VkDeviceSize size_;
			uint32_t bindingIdx_;
			VkDescriptorSetLayoutBinding binding_;
			VkDescriptorBufferInfo bufferInfo_;
			const LogicDevice* logicDevice_;
		};

		template<typename DataType>
		inline void DescriptorBuffer::uploadRange(DataType* data, VkDeviceSize size, VkDeviceSize offset)
		{
			auto deviceMemory = logicDevice_->getDeviceMemory();
			switch (bufferDetails_.access) {
			case MemoryAccessType::kPersistant:
				memcpy(bufferDetails_.mappedData, &data[offset], size * sizeof(DataType));
				break;
			case MemoryAccessType::kDirect: {
				void* dataMap = deviceMemory->mapMemory(bufferDetails_.id);
				memcpy(dataMap, &data[offset], size * sizeof(DataType));
				deviceMemory->unmapMemory(bufferDetails_.id);
				break;
			}
			case MemoryAccessType::kTransfer:
				ASSERT(false);
				break;
			}
		}

		class StorageBuffer : public DescriptorBuffer {
			template<typename T>
			friend DescriptorBuffer* DescriptorBuffer::makeBuffer(const LogicDevice* logicDevice, MemoryAllocationPattern pattern, uint32_t binding,
				VkBufferUsageFlags flags, VkDeviceSize maxSize, VkShaderStageFlags stageFlags);
		protected:
			StorageBuffer(const LogicDevice* logicDevice, uint32_t binding, VkDeviceSize maxSize)
				: DescriptorBuffer(logicDevice, binding, maxSize) {}

			virtual VkBufferUsageFlagBits getUsageBits() override {
				return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			virtual VkDescriptorType getType() override {
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}
		};

		class UniformBuffer : public DescriptorBuffer {
			template<typename T>
			friend DescriptorBuffer* DescriptorBuffer::makeBuffer(const LogicDevice* logicDevice, MemoryAllocationPattern pattern, uint32_t binding,
				VkBufferUsageFlags flags, VkDeviceSize maxSize, VkShaderStageFlags stageFlags);
		protected:
			UniformBuffer(const LogicDevice* logicDevice, uint32_t binding, VkDeviceSize maxSize)
				: DescriptorBuffer(logicDevice, binding, maxSize) {
				
			}

			virtual VkBufferUsageFlagBits getUsageBits() override {
				return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			}
			virtual VkDescriptorType getType() override {
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			}
		};

		class DynamicUniformBuffer : public DescriptorBuffer {
			template<typename T>
			friend DescriptorBuffer* DescriptorBuffer::makeBuffer(const LogicDevice* logicDevice, MemoryAllocationPattern pattern, uint32_t binding,
				VkBufferUsageFlags flags, VkDeviceSize maxSize, VkShaderStageFlags stageFlags);
		protected:
			DynamicUniformBuffer(const LogicDevice* logicDevice, uint32_t binding, VkDeviceSize maxSize)
				: DescriptorBuffer(logicDevice, binding, maxSize) {}

			virtual VkBufferUsageFlagBits getUsageBits() override {
				return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			}
			virtual VkDescriptorType getType() override {
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			}
		};
	}
}
