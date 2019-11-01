// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"
#include "Mesh.h"
#include "AtmosphereRenderer.h"
#include "TerrainRenderer.h"
#include "GraphicsMaster.h"
#include "Image.h"
#include "LogicalCamera.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class Descriptor;
		struct SwapChainDetails;
		class GlobalRenderData;
		class DescriptorBuffer;
		class TextureSampler;

		class RenderPass {
			friend class SwapChain;
		protected:
			struct CreateInfo {
				std::vector<VkAttachmentDescription> attachments;
				std::vector<VkSubpassDescription> subpasses;
				std::vector<VkSubpassDependency> dependencies;
			};

			virtual void doFrame(LogicalCamera& camera, const uint32_t& idx, VkCommandBuffer cmdBuffer) = 0;
			virtual void createRenderers() = 0;
			virtual void initRenderPassDependency(std::vector<Image*> dependencyAttachment) = 0;
			RenderPass(GraphicsMaster* master, LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, GlobalRenderData* grd);
			virtual ~RenderPass();

			void createRenderPass(CreateInfo& createInfo, std::vector<VkImageView>& attachmentImages, bool firstAttachmentIsSwapChainImage);

			void createFramebuffers(LogicDevice* logicDevice, const SwapChainDetails& swapChainDetails, std::vector<VkImageView>& attachmentImages, bool firstAttachmentIsSwapChainImage);
			VkRenderPassBeginInfo beginInfo(const uint32_t& idx);

			VkAttachmentDescription makeAttachment(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
				VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout);
			VkSubpassDescription makeSubpass(VkPipelineBindPoint pipelineType, std::vector<VkAttachmentReference>& colourReferences, VkAttachmentReference* depthReference);
			VkSubpassDependency makeSubpassDependency(uint32_t srcIdx, uint32_t dstIdx, VkPipelineStageFlags srcStage,
				VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess);

			VkRenderPass renderPass_;
			std::vector<VkFramebuffer> framebuffers_;
			const SwapChainDetails& swapChainDetails_;
			LogicDevice* logicDevice_;
			GraphicsMaster* graphicsMaster_;
			Descriptor* descriptor_;
			GlobalRenderData* globalRenderData_;
		};
	}
}
