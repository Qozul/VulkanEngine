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
		class RenderObject;
		class GraphicsComponent;
		class ElementBufferObject;
		class DescriptorBuffer;
		class GlobalRenderData;
		struct BasicMesh;
		struct LogicalCamera;
		struct SceneGraphicsInfo;

		struct RendererCreateInfo2 {
			ElementBufferObject* ebo = nullptr;
			PipelineCreateInfo pipelineCreateInfo;
			std::vector<ShaderStageInfo> shaderStages;
			VkPushConstantRange* pcRanges = nullptr;
			uint32_t pcRangesCount = 0;
			VertexTypes vertexTypes = VertexTypes::VERTEX;
			RendererPipeline::PrimitiveType tessellationPrims = RendererPipeline::PrimitiveType::kQuads;
		};

		struct DescriptorOffsets {
			uint32_t mvp = 0;
			uint32_t params = 0;
			uint32_t material = 0;
		};

		struct ElementData {
			glm::mat4 modelMatrix;
			glm::mat4 mvpMatrix;
		};

		struct PushConstantInfo {
			uint32_t size = 0;
			VkShaderStageFlagBits stages;
			VkMemoryBarrier barrier;
			uint32_t offset = 0;
		};

		struct TessellationPushConstants {
			float distanceFarMinusClose = 0.0f;
			float closeDistance = 0.0f;
			float patchRadius = 0.0f;
			float maxTessellationWeight = 0.0f;
			std::array<glm::vec4, 6> frustumPlanes;
		};

		struct CameraPushConstants {
			glm::vec4 cameraPosition;
		};

		struct VertexPushConstants {
			glm::mat4 shadowMatrix;
			glm::vec4 cameraPosition;
			glm::vec3 mainLightPosition;
			uint32_t shadowTextureIdx = 0;
		};

		struct FragmentPushConstants {
			float screenWidth = 0.0f;
			float screenHeight = 0.0f;
			float screenX = 0.0f;
			float screenY = 0.0f;
		};

		constexpr uint32_t kMaxPushConstantSize = 128;

		class RendererBase {
		public:
			RendererBase(LogicDevice* logicDevice, ElementBufferObject* ebo, SceneGraphicsInfo* graphicsInfo);

			virtual ~RendererBase();
			virtual void recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList, bool ignoreEboBind = false) = 0;
			std::vector<VkWriteDescriptorSet> getDescriptorWrites(uint32_t frameIdx);

			ElementBufferObject* getElementBuffer();
			VkPipelineLayout getPipelineLayout();

			void preframeSetup();
			virtual void toggleWiremeshMode();

			static VkSpecializationMapEntry makeSpecConstantEntry(uint32_t id, uint32_t offset, size_t size);
			static VkSpecializationInfo setupSpecConstants(uint32_t entryCount, VkSpecializationMapEntry* entryPtr, size_t dataSize, const void* data);

			static const VkPushConstantRange setupPushConstantRange(VkShaderStageFlagBits stage, uint32_t size, uint32_t offset);

			template<typename T, typename... Args>
			static const VkSpecializationInfo setupSpecConstantRanges(std::vector<VkSpecializationMapEntry>& entries, const T* constants, const Args... dataEntries);
			template<typename T, typename... Args>
			static void buildSpecMapEntries(std::vector<VkSpecializationMapEntry>& entries, const uint32_t id, const uint32_t offset, const T& current, const Args&... next);
			template<typename T>
			static void buildSpecMapEntries(std::vector<VkSpecializationMapEntry>& entries, const uint32_t id, const uint32_t offset, const T& current);

			template<typename PC>
			const VkPushConstantRange setupPushConstantRange(VkShaderStageFlagBits stages);

			template<typename PC>
			static const std::pair<VkPushConstantRange, PushConstantInfo> createPushConstantRange(VkShaderStageFlagBits stages, uint32_t offset);
		protected:
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages,
				PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount);

			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages,
				PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount, VertexTypes vertexType);

			void beginFrame(VkCommandBuffer& cmdBuffer);
			void bindEBO(VkCommandBuffer& cmdBuffer, uint32_t idx);

			LogicDevice* logicDevice_;
			RendererPipeline* pipeline_;
			ElementBufferObject* ebo_;
			Descriptor* descriptor_;
			SceneGraphicsInfo* graphicsInfo_;
			std::vector<DescriptorBuffer*> storageBuffers_;
			std::vector<VkDescriptorSetLayout> pipelineLayouts_;
			std::vector<VkDescriptorSet> descriptorSets_;
			std::vector<PushConstantInfo> pushConstantInfos_;
			uint32_t pushConstantOffset_;
		};

		template<typename T, typename... Args>
		inline const VkSpecializationInfo RendererBase::setupSpecConstantRanges(std::vector<VkSpecializationMapEntry>& entries, const T* constants, const Args... dataEntries)
		{
			buildSpecMapEntries(entries, 0, 0, dataEntries...);
			return setupSpecConstants(uint32_t(entries.size()), entries.data(), sizeof(T), constants);
		}

		template<typename T, typename... Args>
		inline void RendererBase::buildSpecMapEntries(std::vector<VkSpecializationMapEntry>& entries, const uint32_t id, const uint32_t offset, const T& current, const Args&... next)
		{
			entries.push_back(makeSpecConstantEntry(id, offset, sizeof(current)));
			buildSpecMapEntries(entries, id + 1, offset + sizeof(current), next...);
		}

		template<typename T>
		inline void RendererBase::buildSpecMapEntries(std::vector<VkSpecializationMapEntry>& entries, const uint32_t id, const uint32_t offset, const T& current)
		{
			entries.push_back(makeSpecConstantEntry(id, offset, sizeof(current)));
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
	}
}
