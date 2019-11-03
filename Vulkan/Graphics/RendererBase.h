// Author: Ralph Ridley
// Date: 01/11/19
#pragma once

#include "Mesh.h"
#include "RendererPipeline.h"
#include "StorageBuffer.h"
#include "RenderStorage.h"
#include "GraphicsComponent.h"
#include "GlobalRenderData.h"
#include "Image.h"
#include "LogicalCamera.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class Descriptor;

		struct RendererCreateInfo {
			LogicDevice* logicDevice;
			Descriptor* descriptor;
			GlobalRenderData* globalRenderData;
			VkRenderPass renderPass;
			uint32_t subpassIndex;
			VkExtent2D extent;
			uint32_t maxDrawnEntities;
			uint32_t swapChainImageCount;
			std::string vertexShader;
			std::string fragmentShader;
			std::string geometryShader;
			std::string tessControlShader;
			std::string tessEvalShader;

			void updateRendererSpecific(uint32_t subpassIdx, uint32_t maxEntities, std::string vert, std::string frag, 
				std::string geom = "", std::string tesc = "", std::string tese = "") {
				subpassIndex = subpassIdx;
				maxDrawnEntities = maxEntities;
				vertexShader = vert;
				fragmentShader = frag;
				geometryShader = geom;
				tessControlShader = tesc;
				tessEvalShader = tese;
			}
		};

		struct ElementData {
			glm::mat4 modelMatrix;
			glm::mat4 mvpMatrix;
		};

		struct PushConstantInfo {
			size_t size;
			VkShaderStageFlagBits stages;
			VkMemoryBarrier barrier;
			uint32_t offset;
		};
		constexpr size_t kMaxPushConstantSize = 128;

		class RendererBase {
		public:
			RendererBase(RendererCreateInfo& createInfo, RenderStorage* renderStorage)
				: pipeline_(nullptr), renderStorage_(renderStorage), logicDevice_(createInfo.logicDevice), descriptor_(createInfo.descriptor), pushConstantOffset_(0) {
				ASSERT(createInfo.maxDrawnEntities > 0);
			}
			virtual ~RendererBase();
			virtual void createDescriptors(const uint32_t count) = 0;
			virtual void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) = 0;
			std::vector<VkWriteDescriptorSet> getDescriptorWrites(uint32_t frameIdx);

			void registerComponent(GraphicsComponent* component, RenderObject* robject) {
				renderStorage_->addMesh(component, robject);
			}
			ElementBufferObject* getElementBuffer() {
				return renderStorage_->buffer();
			}
			void preframeSetup() {
				if (renderStorage_ != nullptr) {
					renderStorage_->buffer()->commit();
				}
			}
			void toggleWiremeshMode() {
				pipeline_->switchMode();
			}

			template<typename PC>
			const VkPushConstantRange setupPushConstantRange(VkShaderStageFlagBits stages);

			template<typename PC>
			static const std::pair<VkPushConstantRange, PushConstantInfo> createPushConstantRange(VkShaderStageFlagBits stages, uint32_t offset);
		protected:
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages,
				PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount = RendererPipeline::PrimitiveType::NONE);
			template<typename V>
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages, 
				PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount = RendererPipeline::PrimitiveType::NONE);

			void beginFrame(VkCommandBuffer cmdBuffer);

			LogicDevice* logicDevice_;
			RendererPipeline* pipeline_;
			RenderStorage* renderStorage_;
			Descriptor* descriptor_;
			std::vector<DescriptorBuffer*> storageBuffers_;
			std::vector<VkDescriptorSetLayout> pipelineLayouts_;
			std::vector<VkDescriptorSet> descriptorSets_;
			std::vector<PushConstantInfo> pushConstantInfos_;
			uint32_t pushConstantOffset_;
		};

		inline RendererBase::~RendererBase() {
			for (auto& buffer : storageBuffers_) {
				SAFE_DELETE(buffer);
			}
			SAFE_DELETE(renderStorage_);
			SAFE_DELETE(pipeline_);
		}

		inline std::vector<VkWriteDescriptorSet> RendererBase::getDescriptorWrites(uint32_t frameIdx)
		{
			std::vector<VkWriteDescriptorSet> writes;
			for (auto& buf : storageBuffers_) {
				writes.push_back(buf->descriptorWrite(descriptorSets_[frameIdx]));
			}
			return writes;
		}

		template<typename PC>
		inline const VkPushConstantRange RendererBase::setupPushConstantRange(VkShaderStageFlagBits stages)
		{
			auto pcr = createPushConstantRange<PC>(stages, pushConstantOffset_);
			pushConstantInfos_.push_back(pcr.second);
			pushConstantOffset_ += sizeof(PC);
			return pcr.first;
		}

		template<typename PC>
		inline const std::pair<VkPushConstantRange, PushConstantInfo> RendererBase::createPushConstantRange(VkShaderStageFlagBits stages, uint32_t offset)
		{
			static_assert(sizeof(PC) <= kMaxPushConstantSize, "Push constant size is potentially beyond the limit.");
			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.size = sizeof(PC);
			pushConstantRange.offset = offset;
			pushConstantRange.stageFlags = stages;

			PushConstantInfo pinfo;
			pinfo.size = sizeof(PC);
			pinfo.stages = stages;
			pinfo.barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			pinfo.barrier.pNext = NULL;
			pinfo.barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			pinfo.barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			pinfo.offset = offset;

			return { pushConstantRange, pinfo };
		}

		template<typename V>
		inline void RendererBase::createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages, 
			PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount)
		{
			auto bindingDesc = V::getBindDesc(0, VK_VERTEX_INPUT_RATE_VERTEX);
			auto attribDesc = V::getAttribDescs(0);
			pipelineCreateInfo.vertexInputInfo = RendererPipeline::makeVertexInputInfo<V>(bindingDesc, attribDesc);
			pipeline_ = new RendererPipeline(logicDevice, renderPass, layoutInfo, stages, pipelineCreateInfo, patchVertexCount);
		}

		inline void RendererBase::createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages,
			PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount) {
			pipelineCreateInfo.vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0, 0, nullptr, 0, nullptr };
			pipeline_ = new RendererPipeline(logicDevice, renderPass, layoutInfo, stages, pipelineCreateInfo, patchVertexCount);
		}

		inline void RendererBase::beginFrame(VkCommandBuffer cmdBuffer)
		{
			EXPECTS(pipeline_ != nullptr);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getPipeline());
		}
	}
}
