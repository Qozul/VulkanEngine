// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class ParticleRenderer : public RendererBase {
		public:
			ParticleRenderer(RendererCreateInfo& createInfo, uint32_t maxUniqueParticles);
			~ParticleRenderer();
			void createDescriptors(const uint32_t particleSystemCount) override;
			void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) override;
		private:
			size_t staticParamsDescriptorSet_;
		};
	}
}
