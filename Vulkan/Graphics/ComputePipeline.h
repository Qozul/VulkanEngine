// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;

		class ComputePipeline {
		public:
			ComputePipeline(const LogicDevice* logicDevice, VkPipelineLayoutCreateInfo layoutInfo, const std::string& computeShader);
			~ComputePipeline();

			VkPipeline getPipeline();
			VkPipelineLayout getLayout();
			static VkPipelineLayoutCreateInfo makeLayoutInfo(const uint32_t layoutCount, const VkDescriptorSetLayout* layouts, std::vector<VkPushConstantRange> pushConstantRanges);
		protected:
			VkPipeline pipeline_;
			VkPipelineLayout layout_;
			const LogicDevice* logicDevice_;
		};
	}
}
