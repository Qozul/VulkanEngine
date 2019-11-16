// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"
#include "RendererPipeline.h"
#include "DrawElementsCommand.h"
#include "Vertex.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class Descriptor;
		class RenderStorage;
		class RenderObject;
		class GraphicsComponent;
		class ElementBufferObject;
		class DescriptorBuffer;
		class GlobalRenderData;
		struct BasicMesh;
		struct LogicalCamera;
		struct SceneGraphicsInfo;

		struct RendererCreateInfo {
			LogicDevice* logicDevice;
			Descriptor* descriptor;
			GlobalRenderData* globalRenderData;
			SceneGraphicsInfo* graphicsInfo;
			VkRenderPass renderPass;
			uint32_t subpassIndex;
			VkExtent2D extent;
			uint32_t maxDrawnEntities;
			size_t swapChainImageCount;
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

		struct DescriptorOffsets {
			uint32_t mvp;
			uint32_t params;
			uint32_t material;
		};

		struct ElementData {
			glm::mat4 modelMatrix;
			glm::mat4 mvpMatrix;
		};

		struct PushConstantInfo {
			uint32_t size;
			VkShaderStageFlagBits stages;
			VkMemoryBarrier barrier;
			uint32_t offset;
		};
		constexpr uint32_t kMaxPushConstantSize = 128;

		class RendererBase {
		public:
			RendererBase(RendererCreateInfo& createInfo, RenderStorage* renderStorage)
				: pipeline_(nullptr), renderStorage_(renderStorage), logicDevice_(createInfo.logicDevice), descriptor_(createInfo.descriptor), pushConstantOffset_(0),
				  graphicsInfo_(createInfo.graphicsInfo) { ASSERT(createInfo.maxDrawnEntities > 0); }

			virtual ~RendererBase();
			virtual void createDescriptors(const uint32_t count) = 0;
			virtual void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer) = 0;
			std::vector<VkWriteDescriptorSet> getDescriptorWrites(uint32_t frameIdx);

			virtual void registerComponent(GraphicsComponent* component, RenderObject* robject);
			ElementBufferObject* getElementBuffer();
			VkPipelineLayout getPipelineLayout();

			void preframeSetup();
			virtual void toggleWiremeshMode();

			VkSpecializationMapEntry makeSpecConstantEntry(uint32_t id, uint32_t offset, size_t size);
			VkSpecializationInfo setupSpecConstants(uint32_t entryCount, VkSpecializationMapEntry* entryPtr, size_t dataSize, const void* data);

			template<typename PC>
			const VkPushConstantRange setupPushConstantRange(VkShaderStageFlagBits stages);

			template<typename PC>
			static const std::pair<VkPushConstantRange, PushConstantInfo> createPushConstantRange(VkShaderStageFlagBits stages, uint32_t offset);
		protected:
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages,
				PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount = RendererPipeline::PrimitiveType::kNone);

			template<typename V>
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages, 
				PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount = RendererPipeline::PrimitiveType::kNone);

			void beginFrame(VkCommandBuffer& cmdBuffer);
			void bindEBO(VkCommandBuffer& cmdBuffer, uint32_t idx);

			LogicDevice* logicDevice_;
			RendererPipeline* pipeline_;
			RenderStorage* renderStorage_;
			Descriptor* descriptor_;
			SceneGraphicsInfo* graphicsInfo_;
			std::vector<DescriptorBuffer*> storageBuffers_;
			std::vector<VkDescriptorSetLayout> pipelineLayouts_;
			std::vector<VkDescriptorSet> descriptorSets_;
			std::vector<PushConstantInfo> pushConstantInfos_;
			uint32_t pushConstantOffset_;
		};

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
			pushConstantRange.size = static_cast<uint32_t>(sizeof(PC));
			pushConstantRange.offset = offset;
			pushConstantRange.stageFlags = stages;

			PushConstantInfo pinfo;
			pinfo.size = static_cast<uint32_t>(sizeof(PC));
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
			auto bindingDesc = makeVertexBindingDescription(0, sizeof(V), VK_VERTEX_INPUT_RATE_VERTEX);
			auto attribDesc = makeVertexAttribDescriptions(0, V::makeAttribInfo());
			pipelineCreateInfo.vertexInputInfo = RendererPipeline::makeVertexInputInfo(bindingDesc, attribDesc);
			pipeline_ = new RendererPipeline(logicDevice, renderPass, layoutInfo, stages, pipelineCreateInfo, patchVertexCount);
		}
	}
}
