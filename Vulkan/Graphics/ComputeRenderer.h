#pragma once
#include "RendererBase.h"
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class ComputePipeline;

		class ComputeRenderer : public RendererBase {
		public:
			ComputeRenderer(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount);
			~ComputeRenderer();
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void initialise(const glm::mat4& viewMatrix) override;
			void recordCompute(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
		private:
			ComputePipeline* computePipeline_;
			VkBufferMemoryBarrier bufMemoryBarrier_;
			VkMemoryBarrier pushConstantBarrier_;
			VkBufferMemoryBarrier bufMemoryBarrier2_;
			VkBufferMemoryBarrier transBufMemoryBarrier_;
		};
	}
}
