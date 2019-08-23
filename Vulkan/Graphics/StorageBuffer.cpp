#include "StorageBuffer.h"

using namespace QZL;
using namespace QZL::Graphics;

void DescriptorBuffer::init(MemoryAllocationPattern pattern, VkBufferUsageFlags flags, VkShaderStageFlags stageFlags)
{
	bufferDetails_ = logicDevice_->getDeviceMemory()->createBuffer(pattern, getUsageBits(), size_);
	// TODO create staging buffer transfer alternative
	//ENSURES(bufferDetails_.access == MemoryAccessType::kDirect || bufferDetails_.access == MemoryAccessType::kPersistant);

	binding_ = {};
	binding_.binding = bindingIdx_;
	binding_.descriptorCount = 1;
	binding_.descriptorType = getType();
	binding_.pImmutableSamplers = nullptr;
	binding_.stageFlags = stageFlags;
}

DescriptorBuffer::~DescriptorBuffer()
{
	logicDevice_->getDeviceMemory()->deleteAllocation(bufferDetails_.id, bufferDetails_.buffer);
}

const VkDescriptorSetLayoutBinding& DescriptorBuffer::getBinding()
{
	return binding_;
}

VkWriteDescriptorSet DescriptorBuffer::descriptorWrite(VkDescriptorSet set)
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
	descriptorWrite.descriptorType = getType();
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo_;

	return descriptorWrite;
}

void* DescriptorBuffer::bindRange()
{
	return logicDevice_->getDeviceMemory()->mapMemory(bufferDetails_.id);
}

void DescriptorBuffer::unbindRange()
{
	logicDevice_->getDeviceMemory()->unmapMemory(bufferDetails_.id);
}

DescriptorBuffer::DescriptorBuffer(const LogicDevice* logicDevice, uint32_t binding, VkDeviceSize maxSize)
	: logicDevice_(logicDevice), size_(maxSize), bindingIdx_(binding)
{
}

