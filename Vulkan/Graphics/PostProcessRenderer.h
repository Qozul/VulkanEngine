#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureManager;
		class TextureSampler;

		// Draws atmospheres from within the atmosphere
		class PostProcessRenderer : public RendererBase {
		public:
			PostProcessRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData,
				TextureSampler* geometryColourBuffer, TextureSampler* geometryDepthBuffer);
			~PostProcessRenderer();
			void createDescriptors(const uint32_t entityCount) override;
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
		private:
			TextureSampler* geometryColourBuffer_;
			TextureSampler* geometryDepthBuffer_;
			Descriptor* descriptor_;
		};
	}
}
