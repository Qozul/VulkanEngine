// Author: Ralph Ridley
// Date: 18/11/19
#pragma once
#include "RenderPass.h"

#define SHADOW_DIMENSIONS 2048

namespace QZL {
	namespace Graphics {
		class RendererBase;
		class ShadowPass : public RenderPass {
			friend class SwapChain;
		protected:
			ShadowPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo);
			~ShadowPass();
			void doFrame(LogicalCamera& camera, const uint32_t& idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandLists) override;
			void createRenderers() override;
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override { }
		private:
			VkFormat createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);

			RendererBase* shadowRenderer_;
			Image* depthBuffer_;
		};
	}
}
