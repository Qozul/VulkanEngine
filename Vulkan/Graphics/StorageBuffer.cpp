#include "StorageBuffer.h"

using namespace QZL;
using namespace QZL::Graphics;

StorageBuffer::StorageBuffer(const LogicDevice* logicDevice, MemoryAllocationPattern pattern,
	uint32_t bindingIdx, VkBufferUsageFlags flags, VkDeviceSize maxSize, VkShaderStageFlags stageFlags, bool uniform)
	: logicDevice_(logicDevice), size_(maxSize), bindingIdx_(bindingIdx), uniform_(uniform)
{
	if (uniform) {
		bufferDetails_ = logicDevice_->getDeviceMemory()->createBuffer(pattern, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, maxSize);
	}
	else {
		bufferDetails_ = logicDevice_->getDeviceMemory()->createBuffer(pattern, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, maxSize);
	}
	// TODO create staging buffer transfer alternative
	//ENSURES(bufferDetails_.access == MemoryAccessType::kDirect || bufferDetails_.access == MemoryAccessType::kPersistant);

	binding_ = {};
	binding_.binding = bindingIdx_;
	binding_.descriptorCount = 1;
	if (uniform) {
		binding_.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
	else {
		binding_.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}
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
	if (uniform_) {
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
	else {
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}
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

