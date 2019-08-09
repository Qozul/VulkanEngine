#include "RendererPipeline.h"
#include "LogicDevice.h"
#include "Shader.h"
#include "Vertex.h"

using namespace QZL;
using namespace QZL::Graphics;

RendererPipeline::RendererPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, 
	VkPipelineLayoutCreateInfo layoutInfo, VkPipelineVertexInputStateCreateInfo& vertexInputInfo, const std::string& vertexShader, const std::string& fragmentShader, 
	VkFrontFace frontFace)
	: logicDevice_(logicDevice), layout_(VK_NULL_HANDLE)
{
	Shader vertexModule = { *logicDevice_, vertexShader };
	Shader fragmentModule = { *logicDevice_, fragmentShader };
	std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfo = {
		createShaderInfo(vertexModule.getModule(), VK_SHADER_STAGE_VERTEX_BIT),
		createShaderInfo(fragmentModule.getModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	createPipeline(logicDevice, renderPass, swapChainExtent, layoutInfo, shaderStagesInfo, createInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE), nullptr, vertexInputInfo, frontFace);
}

RendererPipeline::RendererPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, VkPipelineLayoutCreateInfo layoutInfo, 
	VkPipelineVertexInputStateCreateInfo& vertexInputInfo, const std::string& vertexShader, const std::string& fragmentShader, const std::string& tessCtrlShader, 
	const std::string& tessEvalShader, PrimitiveType patchVertexCount, VkFrontFace frontFace)
	: logicDevice_(logicDevice), layout_(VK_NULL_HANDLE)
{
	Shader vertexModule = { *logicDevice_, vertexShader };
	Shader tessCtrlModule = { *logicDevice_, tessCtrlShader };
	Shader tessEvalModule = { *logicDevice_, tessEvalShader };
	Shader fragmentModule = { *logicDevice_, fragmentShader };
	std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfo = {
		createShaderInfo(vertexModule.getModule(), VK_SHADER_STAGE_VERTEX_BIT),
		createShaderInfo(fragmentModule.getModule(), VK_SHADER_STAGE_FRAGMENT_BIT),
		createShaderInfo(tessCtrlModule.getModule(), VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT),
		createShaderInfo(tessEvalModule.getModule(), VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
	};
	auto tessellationInfo = createTessellationStateInfo(patchVertexCount);
	createPipeline(logicDevice, renderPass, swapChainExtent, layoutInfo, shaderStagesInfo, createInputAssembly(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST, VK_FALSE), &tessellationInfo, vertexInputInfo, frontFace);
}

RendererPipeline::~RendererPipeline()
{
	vkDestroyPipeline(*logicDevice_, pipeline_, nullptr);
	vkDestroyPipelineLayout(*logicDevice_, layout_, nullptr);
	vkDestroyPipeline(*logicDevice_, wiremeshPipeline_, nullptr);
	vkDestroyPipelineLayout(*logicDevice_, wiremeshLayout_, nullptr);
}

void RendererPipeline::switchMode()
{
	wiremeshMode_ = !wiremeshMode_;
}

VkPipeline RendererPipeline::getPipeline()
{
	if (wiremeshMode_) {
		return wiremeshPipeline_;
	}
	else {
		return pipeline_;
	}
}

VkPipelineLayout RendererPipeline::getLayout()
{
	if (wiremeshMode_) {
		return wiremeshLayout_;
	}
	else {
		return layout_;
	}
}

VkPipelineLayoutCreateInfo RendererPipeline::makeLayoutInfo(const uint32_t layoutCount, const VkDescriptorSetLayout* layouts)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = layoutCount;
	pipelineLayoutInfo.pSetLayouts = layouts;
	return pipelineLayoutInfo;
}

void RendererPipeline::createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, VkPipelineLayoutCreateInfo layoutInfo, 
	std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfo, VkPipelineInputAssemblyStateCreateInfo inputAssembly, VkPipelineTessellationStateCreateInfo* tessellationInfo,
	VkPipelineVertexInputStateCreateInfo& vertexInputInfo, VkFrontFace frontFace)
{
	wiremeshMode_ = false;
	
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = frontFace;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	CHECK_VKRESULT(vkCreatePipelineLayout(*logicDevice_, &layoutInfo, nullptr, &layout_));

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStagesInfo.size();
	pipelineInfo.pStages = shaderStagesInfo.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = layout_;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (inputAssembly.topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST && tessellationInfo != nullptr) {
		pipelineInfo.pTessellationState = tessellationInfo;
	}

	CHECK_VKRESULT(vkCreateGraphicsPipelines(*logicDevice_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_));

	// Wiremesh mode for debugging
	rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	CHECK_VKRESULT(vkCreatePipelineLayout(*logicDevice_, &layoutInfo, nullptr, &wiremeshLayout_));
	CHECK_VKRESULT(vkCreateGraphicsPipelines(*logicDevice_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &wiremeshPipeline_));
}

VkPipelineShaderStageCreateInfo RendererPipeline::createShaderInfo(VkShaderModule module, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage = stage;
	info.module = module;
	info.pName = "main";
	return info;
}

VkPipelineInputAssemblyStateCreateInfo RendererPipeline::createInputAssembly(VkPrimitiveTopology topology, VkBool32 enablePrimitiveRestart)
{
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = topology;
	inputAssembly.primitiveRestartEnable = enablePrimitiveRestart;
	return inputAssembly;
}

VkPipelineTessellationStateCreateInfo RendererPipeline::createTessellationStateInfo(PrimitiveType patchPointCount)
{
	VkPipelineTessellationStateCreateInfo tessellationInfo = {};
	tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationInfo.patchControlPoints = (uint32_t)patchPointCount;
	return tessellationInfo;
}
