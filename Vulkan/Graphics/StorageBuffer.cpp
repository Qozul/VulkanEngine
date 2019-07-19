#include "StorageBuffer.h"

using namespace QZL;
using namespace QZL::Graphics;

StorageBuffer::StorageBuffer(const LogicDevice* logicDevice, MemoryAllocationPattern pattern,
	uint32_t bindingIdx, VkBufferUsageFlags flags, VkDeviceSize maxSize, VkShaderStageFlags stageFlags)
	: logicDevice_(logicDevice), size_(maxSize), bindingIdx_(bindingIdx)
{
	bufferDetails_ = logicDevice_->getDeviceMemory()->createBuffer(pattern, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, maxSize);
	// TODO create staging buffer transfer alternative
	//ENSURES(bufferDetails_.access == MemoryAccessType::kDirect || bufferDetails_.access == MemoryAccessType::kPersistant);

	binding_ = {};
	binding_.binding = bindingIdx_;
	binding_.descriptorCount = 1;
	binding_.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	binding_.pImmutableSamplers = nullptr;
	binding_.stageFlags = stageFlags;
}

StorageBuffer::~StorageBuffer()
{
	logicDevice_->getDeviceMemory()->deleteAllocation(bufferDetails_.id, bufferDetails_.buffer);
}

const VkDescriptorSetLayoutBinding& StorageBuffer::getBinding()
{
	return binding_;
}

VkWriteDescriptorSet StorageBuffer::descriptorWrite(VkDescriptorSet set)
{
	bufferInfo_ = {};
	bufferInfo_.buffer = bufferDetails_.buffer;
	bufferInfo_.offset = 0;
	bufferInfo_.range = size_;

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = set;
	descriptorWrite.dstBinding = bindingIdx_;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo_;

	return descriptorWrite;
}

void* StorageBuffer::bindRange()
{
	return logicDevice_->getDeviceMemory()->mapMemory(bufferDetails_.id);
}

void StorageBuffer::unbindRange()
{
	logicDevice_->getDeviceMemory()->unmapMemory(bufferDetails_.id);
}

