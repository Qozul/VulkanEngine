#include "ComputePipeline.h"
#include "LogicDevice.h"
#include "Shader.h"

using namespace QZL;
using namespace QZL::Graphics;

ComputePipeline::ComputePipeline(const LogicDevice* logicDevice, VkPipelineLayoutCreateInfo layoutInfo, const std::string& computeShader)
	: logicDevice_(logicDevice)
{
	CHECK_VKRESULT(vkCreatePipelineLayout(*logicDevice_, &layoutInfo, nullptr, &layout_));

	Shader module = { *logicDevice_, computeShader };

	VkPipelineShaderStageCreateInfo stageInfo;
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.pNext = NULL;
	stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageInfo.module = module.getModule();
	stageInfo.pName = "main";
	stageInfo.pSpecializationInfo = nullptr;
	stageInfo.flags = VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT;

	VkComputePipelineCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.stage = stageInfo;
	createInfo.layout = layout_;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	CHECK_VKRESULT(vkCreateComputePipelines(*logicDevice_, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline_));
}

ComputePipeline::~ComputePipeline()
{
	vkDestroyPipeline(*logicDevice_, pipeline_, nullptr);
	vkDestroyPipelineLayout(*logicDevice_, layout_, nullptr);
}

VkPipeline ComputePipeline::getPipeline()
{
	return pipeline_;
}

VkPipelineLayout ComputePipeline::getLayout()
{
	return layout_;
}

VkPipelineLayoutCreateInfo ComputePipeline::makeLayoutInfo(const uint32_t layoutCount, const VkDescriptorSetLayout* layouts, std::vector<VkPushConstantRange> pushConstantRanges)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = layoutCount;
	pipelineLayoutInfo.pSetLayouts = layouts;
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
	return pipelineLayoutInfo;
}
