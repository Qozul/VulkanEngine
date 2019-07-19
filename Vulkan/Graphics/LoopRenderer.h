#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class LoopRenderer : public RendererBase {
		public:
			LoopRenderer(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount);
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void initialise(const glm::mat4& viewMatrix) override;
		};
	}
}
