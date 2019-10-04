#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureManager;
		class TextureSampler;

		// Draws atmospheres from within the atmosphere
		class AtmosphereRenderer : public RendererBase {
		public:
			AtmosphereRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData,
				TextureSampler* geometryColourBuffer, TextureSampler* geometryDepthBuffer);
			~AtmosphereRenderer();
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void initialise(const glm::mat4& viewMatrix) override;
		private:
			TextureSampler* geometryColourBuffer_;
			TextureSampler* geometryDepthBuffer_;
			Descriptor* descriptor_;
		};
	}
}
