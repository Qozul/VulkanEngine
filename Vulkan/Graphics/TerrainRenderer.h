#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class TextureSampler;
		class DeviceMemory;
		class TextureLoader;

		class TerrainRenderer : public RendererBase {
		public:
			TerrainRenderer(const LogicDevice* logicDevice, TextureLoader*& textureLoader, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& tessCtrlShader, const std::string& tessEvalShader, const std::string& fragmentShader, 
				const uint32_t entityCount, const GlobalRenderData* globalRenderData);
			~TerrainRenderer();
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void initialise(const glm::mat4& viewMatrix) override;
		private:
			Descriptor* descriptor_;
		};
	}
}
