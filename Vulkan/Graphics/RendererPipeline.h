#pragma once
#include "VkUtil.h"
#include "Vertex.h"

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
			VkExtent2D extent;
			VkPipelineVertexInputStateCreateInfo vertexInputInfo;
			VkPrimitiveTopology primitiveTopology;
			VkFrontFace frontFace;
			VkBool32 enableDepthTest;
			VkBool32 enableDepthWrite;
		};

		class RendererPipeline {
		public:
			enum class PrimitiveType : uint32_t {
				NONE = 0,
				TRIANGLES = 3,
				QUADS = 4
			};
		public:
			RendererPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages, 
				PipelineCreateInfo pipelineCreateInfo, PrimitiveType patchVertexCount = PrimitiveType::NONE);

			~RendererPipeline();

			void switchMode();
			VkPipeline getPipeline();
			VkPipelineLayout getLayout();
			static VkPipelineLayoutCreateInfo makeLayoutInfo(const uint32_t layoutCount, const VkDescriptorSetLayout* layouts, 
				const uint32_t pushConstantCount = 0, const VkPushConstantRange* pushConstantRange = nullptr);
			template<typename V>
			static VkPipelineVertexInputStateCreateInfo makeVertexInputInfo(VkVertexInputBindingDescription& bindingDesc,
				typename std::result_of<decltype(&V::getAttribDescs)(uint32_t)>::type attribDescs);
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
			/*
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, VkPipelineLayoutCreateInfo layoutInfo,
				std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfo, VkPipelineInputAssemblyStateCreateInfo inputAssembly, VkPipelineTessellationStateCreateInfo* tessellationInfo,
				VkPipelineVertexInputStateCreateInfo& vertexInputInfo, VkFrontFace frontFace, bool enableDepthTest);
			*/
			VkPipelineShaderStageCreateInfo createShaderInfo(VkShaderModule module, VkShaderStageFlagBits stage);
			VkPipelineInputAssemblyStateCreateInfo createInputAssembly(VkPrimitiveTopology topology, VkBool32 enablePrimitiveRestart);
			VkPipelineTessellationStateCreateInfo createTessellationStateInfo(PrimitiveType patchPointCount);
		};
		template<typename V>
		inline VkPipelineVertexInputStateCreateInfo RendererPipeline::makeVertexInputInfo(VkVertexInputBindingDescription& bindingDesc,
			typename std::result_of<decltype(&V::getAttribDescs)(uint32_t)>::type attribDescs)
		{
			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescs.size());
			vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
			vertexInputInfo.pVertexAttributeDescriptions = attribDescs.data();
			return vertexInputInfo;
		}
	}
}
