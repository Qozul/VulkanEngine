#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureSampler;
		class Image;

		// Draws atmospheres from within the atmosphere
		class PostProcessRenderer : public RendererBase {
		public:
			PostProcessRenderer(LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent,
				Descriptor* descriptor, const std::string& vertexShader, const std::string& fragmentShader, Image* apScatteringTex,
				Image* apTransmittanceTex, TextureSampler* colourTex, TextureSampler* depthTex);
			~PostProcessRenderer();
			void createDescriptors(const uint32_t count) override;
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
		private:
			Descriptor* descriptor_;
			TextureSampler* apScatteringTexture_;
			TextureSampler* apTransmittanceTexture_;
			TextureSampler* colourTexture_;
			TextureSampler* depthTexture_;

			
		};
	}
}
