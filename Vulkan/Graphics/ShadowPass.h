// Author: Ralph Ridley
// Date: 18/11/19
#pragma once
#include "RenderPass.h"

#define SHADOW_DIMENSIONS 4096

namespace QZL {
	namespace Graphics {
		class RendererBase;
		class ShadowPass : public RenderPass {
			friend class SwapChain;
		protected:
			ShadowPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo);
			~ShadowPass();
			void doFrame(FrameInfo& frameInfo) override;
			void createRenderers() override;
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override { }
		private:
			void createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			VkFormat createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);

			RendererBase* shadowRenderer_;
			RendererBase* shadowTerrainRenderer_;
			Image* depthBuffer_;
			Image* colourBuffer_;

			uint32_t terrainHeightmapIdx_;
		};
	}
}
