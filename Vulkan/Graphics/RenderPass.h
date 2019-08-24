#pragma once
#include "VkUtil.h"
#include "Mesh.h"

namespace QZL
{
	namespace Graphics {
		class Image2D;
		class LogicDevice;
		class RendererBase;
		class Descriptor;
		struct SwapChainDetails;
		class GraphicsMaster;
		struct GlobalRenderData;
		class DescriptorBuffer;

		class RenderPass {
			friend class SwapChain;

			static const size_t kMaxLights = 1;
			struct LightingData {
				glm::vec4 cameraPosition;
				glm::vec4 ambientColour;
				std::array<glm::vec4, kMaxLights> lightPositions;
			};

			enum class SubPass : uint32_t {
				ATMOSPHERE,
				GENERAL,
				SUBPASS_COUNT
			};
		public:
			void doFrame(const uint32_t idx, VkCommandBuffer cmdBuffer);
		private:
			RenderPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			~RenderPass();

			void createFramebuffers(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			void createBackBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			VkFormat createDepthBuffer(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails);
			void createRenderers();

			void updateGlobalDescriptors(const uint32_t idx, VkCommandBuffer cmdBuffer);

			VkRenderPass renderPass_;
			std::vector<VkFramebuffer> framebuffers_;
			const SwapChainDetails& swapChainDetails_;
			LogicDevice* logicDevice_;
			GraphicsMaster* graphicsMaster_;

			Descriptor* descriptor_;

			RendererBase* texturedRenderer_;
			RendererBase* terrainRenderer_;
			RendererBase* atmosphereRenderer_;

			Image2D* backBuffer_;
			Image2D* depthBuffer_;

			GlobalRenderData* globalRenderData_;
			DescriptorBuffer* lightingUbo_;

			static const size_t kMaxRenderers = 1;
			static const glm::vec3 kAmbientColour;
		};
	}
}
