// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RenderPass.h"

namespace QZL {
	namespace Graphics {
		class RendererBase;
		class ElementBufferObject;
		class GeometryPass : public RenderPass {
			friend class SwapChain;
			enum class SubPass : uint32_t {
				kAtmosphere,
				kGeneral,
				kSubpassCount
			};
		protected:
			GeometryPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo);
			~GeometryPass();
			void doFrame(LogicalCamera* cameras, const size_t cameraCount, const uint32_t& idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandLists) override;
			void createRenderers() override;
			void initRenderPassDependency(std::vector<Image*> dependencyAttachment) override;
		private:
			void createColourBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			VkFormat createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);

			RendererBase* texturedRenderer_;
			RendererBase* terrainRenderer_;
			RendererBase* atmosphereRenderer_;
			RendererBase* particleRenderer_;
			RendererBase* waterRenderer_;

			Image* colourBuffer_;
			Image* msaaResolveBuffer_;
			Image* depthBuffer_;
			Image* shadowDepthBuf_;
			uint32_t shadowDepthTexture_;
		};
	}
}
