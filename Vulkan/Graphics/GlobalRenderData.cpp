// Author: Ralph Ridley
// Date: 04/11/19
#include "GlobalRenderData.h"

using namespace QZL;
using namespace QZL::Graphics;

void GlobalRenderData::updateData(uint32_t idx, LightingData& data)
{
	LightingData* lightingPtr = static_cast<LightingData*>(lightingUbo_->bindRange());
	*lightingPtr = data;
	lightingUbo_->unbindRange();
}

GlobalRenderData::GlobalRenderData(LogicDevice* logicDevice)
{
	createDescriptorSet(logicDevice, { 0, 0 });
}

GlobalRenderData::GlobalRenderData(LogicDevice* logicDevice, VkDescriptorSetLayoutBinding descriptorIndexBinding)
{
	createDescriptorSet(logicDevice, { 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT }, &descriptorIndexBinding);
}

GlobalRenderData::~GlobalRenderData()
{
	SAFE_DELETE(lightingUbo_);
}

void GlobalRenderData::createDescriptorSet(LogicDevice* logicDevice, std::vector<VkDescriptorBindingFlagsEXT> bindingFlags, VkDescriptorSetLayoutBinding* descriptorIndexBinding)
{
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags = {};
	setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	setLayoutBindingFlags.bindingCount = static_cast<uint32_t>(bindingFlags.size());
	setLayoutBindingFlags.pBindingFlags = bindingFlags.data();

	lightingUbo_ = DescriptorBuffer::makeBuffer<UniformBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)GlobalRenderDataBindings::kLighting, 0,
		sizeof(LightingData), VK_SHADER_STAGE_ALL, "GlobalLightingBuffer");

	auto descriptor = logicDevice->getPrimaryDescriptor();
	if (descriptorIndexBinding != nullptr) {
		layout_ = descriptor->makeLayout({ lightingUbo_->getBinding(), *descriptorIndexBinding }, &setLayoutBindingFlags);
	}
	else {
		layout_ = descriptor->makeLayout({ lightingUbo_->getBinding() });
	}
	auto idx = descriptor->createSets({ layout_ });
	set_ = descriptor->getSet(idx);
	descriptor->updateDescriptorSets({ lightingUbo_->descriptorWrite(set_) });
}
