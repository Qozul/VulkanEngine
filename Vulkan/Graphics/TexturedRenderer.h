#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class TextureSampler;
		class DeviceMemory;
		class TextureManager;

		class TexturedRenderer : public RendererBase {
		public:
			TexturedRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData);
			~TexturedRenderer();
			void createDescriptors(const uint32_t count) override;
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void recordDIFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer);
			void recordNormalFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer);
		private:
			void updateBuffers(const glm::mat4& viewMatrix);
			void updateDIBuffer();
			Descriptor* descriptor_;
		};
	}
}
