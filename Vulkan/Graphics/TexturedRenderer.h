// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class TexturedRenderer : public RendererBase {
		public:
			TexturedRenderer(RendererCreateInfo& createInfo);
			~TexturedRenderer();
			void createDescriptors(const uint32_t count) override;
			void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void recordDIFrame(const uint32_t idx, VkCommandBuffer cmdBuffer);
		private:
			void updateBuffers(const glm::mat4& viewMatrix);
			void updateDIBuffer();
		};
	}
}
