#pragma once
#include "LogicDevice.h"
#include "DeviceMemory.h"

namespace QZL
{
	namespace Graphics {
		class StorageBuffer {
		public:
			StorageBuffer(const LogicDevice* logicDevice, MemoryAllocationPattern pattern, uint32_t binding, VkBufferUsageFlags flags, VkDeviceSize maxSize,
				VkShaderStageFlags stageFlags, bool uniform = false);
			~StorageBuffer();
			const VkDescriptorSetLayoutBinding& getBinding();
			VkWriteDescriptorSet descriptorWrite(VkDescriptorSet set);
			template<typename DataType>
			void uploadRange(DataType* data, VkDeviceSize size, VkDeviceSize offset);
			// Alternative to uploading a range directly, these allow the mapping to last longer
			// But if bind is called the caller must ensure unbind is also called
			void* bindRange();
			void unbindRange();
			const MemoryAllocationDetails& getBufferDetails() { return bufferDetails_; }
		private:
			MemoryAllocationDetails bufferDetails_;
			VkDeviceSize size_;
			uint32_t bindingIdx_;
			VkDescriptorSetLayoutBinding binding_;
			VkDescriptorBufferInfo bufferInfo_;
			const LogicDevice* logicDevice_;
			const bool uniform_;
		};

		template<typename DataType>
		inline void StorageBuffer::uploadRange(DataType* data, VkDeviceSize size, VkDeviceSize offset)
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
				// TODO
				ASSERT(false);
				break;
			}
		}
	}
}
