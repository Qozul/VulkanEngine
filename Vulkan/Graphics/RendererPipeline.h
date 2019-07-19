#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;

		class RendererPipeline {
		public:
			RendererPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, VkPipelineLayoutCreateInfo layoutInfo,
				const std::string& vertexShader, const std::string& fragmentShader);
			~RendererPipeline();

			VkPipeline getPipeline();
			VkPipelineLayout getLayout();
			static VkPipelineLayoutCreateInfo makeLayoutInfo(const uint32_t layoutCount, const VkDescriptorSetLayout* layouts);
		protected:
			VkPipeline pipeline_;
			VkPipelineLayout layout_;
			const LogicDevice* logicDevice_;
		};
	}
}
