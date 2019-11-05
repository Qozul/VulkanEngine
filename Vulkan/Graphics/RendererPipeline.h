// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;

		struct ShaderStageInfo {
			std::string name;
			VkShaderStageFlagBits stageFlag;
			VkSpecializationInfo* specConstants;

			ShaderStageInfo(std::string name, VkShaderStageFlagBits stageFlag, VkSpecializationInfo* specConstants)
				: name(name), stageFlag(stageFlag), specConstants(specConstants) { }
		};

		struct PipelineCreateInfo {
			std::string debugName = "";
			VkExtent2D extent;
			VkPipelineVertexInputStateCreateInfo vertexInputInfo;
			VkPrimitiveTopology primitiveTopology;
			VkFrontFace frontFace;
			VkBool32 enableDepthTest;
			VkBool32 enableDepthWrite;
			uint32_t subpassIndex;
		};

		class RendererPipeline {
		public:
			enum class PrimitiveType : uint32_t {
				kNone = 0,
				kTriangles = 3,
				kQuads = 4
			};
		public:
			RendererPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages, 
				PipelineCreateInfo pipelineCreateInfo, PrimitiveType patchVertexCount = PrimitiveType::kNone);

			~RendererPipeline();

			void switchMode();
			VkPipeline getPipeline();
			VkPipelineLayout getLayout();
			static VkPipelineLayoutCreateInfo makeLayoutInfo(const uint32_t layoutCount, const VkDescriptorSetLayout* layouts, 
				const uint32_t pushConstantCount = 0, const VkPushConstantRange* pushConstantRange = nullptr);
			
			static VkPipelineVertexInputStateCreateInfo makeVertexInputInfo(VkVertexInputBindingDescription& bindingDesc,
				std::vector<VkVertexInputAttributeDescription>& attribDescs);
		protected:
			VkPipeline pipeline_;
			VkPipeline wiremeshPipeline_;
			VkPipelineLayout layout_;
			VkPipelineLayout wiremeshLayout_;
			const LogicDevice* logicDevice_;
			bool wiremeshMode_;
		private:
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo,
				std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfo, VkPipelineInputAssemblyStateCreateInfo inputAssembly, VkPipelineTessellationStateCreateInfo* tessellationInfo, PipelineCreateInfo pipelineCreateInfo);
			
			VkPipelineShaderStageCreateInfo createShaderInfo(VkShaderModule module, VkShaderStageFlagBits stage);
			VkPipelineInputAssemblyStateCreateInfo createInputAssembly(VkPrimitiveTopology topology, VkBool32 enablePrimitiveRestart);
			VkPipelineTessellationStateCreateInfo createTessellationStateInfo(PrimitiveType patchPointCount);
		};
	}
}
