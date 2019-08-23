#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class TextureSampler;
		class DeviceMemory;
		class TextureManager;

		class TerrainRenderer : public RendererBase {
			struct TessControlInfo {
				float distanceFarMinusClose;
				float closeDistance;
				float patchRadius;
				float maxTessellationWeight;
				std::array<glm::vec4, 6> frustumPlanes;
			};
		public:
			static DescriptorRequirementMap getDescriptorRequirements();
			static constexpr uint32_t MAX_ENTITIES = 1;

			TerrainRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& tessCtrlShader, const std::string& tessEvalShader, const std::string& fragmentShader, 
				const uint32_t entityCount, const GlobalRenderData* globalRenderData);
			~TerrainRenderer();
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void initialise(const glm::mat4& viewMatrix) override;
		private:
			void updateBuffers(const glm::mat4& viewMatrix);
			std::array<glm::vec4, 6> calculateFrustumPlanes(const glm::mat4& mvp);

			Descriptor* descriptor_;
			TessControlInfo tessCtrlInfo_;
		};
	}
}
