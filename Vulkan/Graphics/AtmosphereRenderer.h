#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureManager;

		class AtmosphereRenderer : public RendererBase {
			struct TessControlInfo {
				float distanceFarMinusClose;
				float closeDistance;
				float patchRadius;
				float maxTessellationWeight;
				std::array<glm::vec4, 6> frustumPlanes;
			};
		public:
			static DescriptorRequirementMap getDescriptorRequirements();
			AtmosphereRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& tessCtrlShader, const std::string& tessEvalShader, const std::string& fragmentShader,
				const uint32_t entityCount, const GlobalRenderData* globalRenderData);
			~AtmosphereRenderer();
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void initialise(const glm::mat4& viewMatrix) override;
		private:
			void updateBuffers(const glm::mat4& viewMatrix);
			std::array<glm::vec4, 6> calculateFrustumPlanes(const glm::mat4 & mvp);

			size_t staticParamsDescriptorSet_;
			Descriptor* descriptor_;
			TessControlInfo tessCtrlInfo_;
		};
	}
}
