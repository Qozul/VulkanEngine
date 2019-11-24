// Author: Ralph Ridley
// Date: 23/11/19
#pragma once
#include "RenderPass.h"

namespace QZL {
	namespace Graphics {
		class RendererBase;
		class LightingPass : public RenderPass {
			friend class SwapChain;
		protected:
			LightingPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo);
			~LightingPass();
			void doFrame(FrameInfo& frameInfo) override;
			void createRenderers() override;
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override;
		private:
			void createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);

			RendererBase* lightingRenderer_;
			RendererBase* ssaoRenderer_;

			Image* diffuseBuffer_;
			Image* specularBuffer_;
			Image* ambientBuffer_;
			uint32_t normalsIdx_;
			uint32_t positionIdx_;
			uint32_t depthIdx_;
			uint32_t shadowDepthIdx_;
		};
	}
}
