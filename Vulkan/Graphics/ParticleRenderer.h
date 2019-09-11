#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureManager;
		class TextureSampler;

		class ParticleRenderer : public RendererBase {
		public:
			ParticleRenderer(LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
				const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const uint32_t particleSystemCount, const GlobalRenderData* globalRenderData,
				glm::vec3* billboardPoint);
			~ParticleRenderer();
			void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void initialise(const glm::mat4& viewMatrix) override;
		private:
			size_t staticParamsDescriptorSet_;
			Descriptor* descriptor_;
			glm::vec3* billboardPoint_;
		};
	}
}
