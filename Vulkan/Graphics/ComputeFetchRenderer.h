#pragma once
#include "RendererBase.h"
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class ComputePipeline;
		class LogicDevice;

		class ComputeFetchRenderer : public RendererBase {
		public:
			ComputeFetchRenderer(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount);
			~ComputeFetchRenderer();
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void initialise(const glm::mat4& viewMatrix) override;
			void recordCompute(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
		private:
			void readback();
			ComputePipeline* computePipeline_;
			VkMemoryBarrier pushConstantBarrier_;

			VkFence readbackFence_;

			const LogicDevice* logicDevice_;
		};
	}
}
