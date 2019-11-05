// Author: Ralph Ridley
// Date: 01/11/19
#include "Descriptor.h"
#include "LogicDevice.h"

using namespace QZL;
using namespace QZL::Graphics;

Descriptor::Descriptor(const LogicDevice* logicDevice, const uint32_t maxSets, std::vector<std::pair<VkDescriptorType, uint32_t>> types)
	: logicDevice_(logicDevice)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (auto type : types) {
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = type.first;
		poolSize.descriptorCount = type.second;
		poolSizes.push_back(poolSize);
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxSets;

	CHECK_VKRESULT(vkCreateDescriptorPool(*logicDevice_, &poolInfo, nullptr, &pool_));
}

Descriptor::~Descriptor()
{
	for (auto layout : layouts_) {
		vkDestroyDescriptorSetLayout(*logicDevice_, layout, nullptr);
	}
	vkDestroyDescriptorPool(*logicDevice_, pool_, nullptr);
	sets_.clear();
}

size_t Descriptor::createSets(const std::vector<VkDescriptorSetLayout>& layouts)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool_;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	size_t firstIdx = sets_.size();
	sets_.resize(sets_.size() + layouts.size());
	CHECK_VKRESULT(vkAllocateDescriptorSets(*logicDevice_, &allocInfo, &sets_[firstIdx]));
	return firstIdx;
}

const VkDescriptorSet Descriptor::getSet(size_t idx)
{
	return sets_[idx];
}

VkDescriptorSetLayout Descriptor::makeLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, const void* pNext)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	layoutInfo.pNext = pNext;
	VkDescriptorSetLayout layout;
	CHECK_VKRESULT(vkCreateDescriptorSetLayout(*logicDevice_, &layoutInfo, nullptr, &layout));
	layouts_.push_back(layout);
	return layout;
}

void Descriptor::updateDescriptorSets(const std::vector<VkWriteDescriptorSet>& descriptorWrites)
{
	vkUpdateDescriptorSets(*logicDevice_, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}
