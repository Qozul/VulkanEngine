// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;
		class TextureSampler;
		class Material;
		class GraphicsComponent;
		struct AtmosphereShaderParams;

		class PostProcessRenderer : public RendererBase {
		public:
			PostProcessRenderer(RendererCreateInfo& createInfo, TextureSampler* geometryColourBuffer, TextureSampler* geometryDepthBuffer);
			~PostProcessRenderer();
			void createDescriptors(const uint32_t entityCount) override;
			void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
			void registerComponent(GraphicsComponent* component, RenderObject* robject) override;
		private:
			TextureSampler* geometryColourBuffer_;
			TextureSampler* geometryDepthBuffer_;
			AtmosphereShaderParams* params_;
			Material* material_;
			GraphicsComponent* component_;
		};
	}
}
