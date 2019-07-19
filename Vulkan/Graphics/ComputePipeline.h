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
			static VkPipelineLayoutCreateInfo makeLayoutInfo(const uint32_t layoutCount, const VkDescriptorSetLayout* layouts);
		protected:
			VkPipeline pipeline_;
			VkPipelineLayout layout_;
			const LogicDevice* logicDevice_;
		};
	}
}
