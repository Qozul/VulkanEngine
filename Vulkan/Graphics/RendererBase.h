#pragma once
#include "Mesh.h"
#include "RendererPipeline.h"
#include "StorageBuffer.h"
#include "RenderStorage.h"
#include "GraphicsComponent.h"
#include "ElementBuffer.h"
#include "GlobalRenderData.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class Descriptor;

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
		constexpr size_t MAX_PUSH_CONSTANT_SIZE = 128;

		class RendererBase {
		public:
			RendererBase(LogicDevice* logicDevice, RenderStorage* renderStorage)
				: pipeline_(nullptr), renderStorage_(renderStorage), logicDevice_(logicDevice), pushConstantOffset_(0) {
			}
			RendererBase(LogicDevice* logicDevice)
				: pipeline_(nullptr), renderStorage_(nullptr), logicDevice_(logicDevice), pushConstantOffset_(0) {
			}
			virtual ~RendererBase();
			virtual void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) = 0;
			virtual void recordCompute(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer);
			std::vector<VkWriteDescriptorSet> getDescriptorWrites(uint32_t frameIdx);
			virtual void initialise(const glm::mat4& viewMatrix) = 0;

			void registerComponent(GraphicsComponent* component, RenderObject* robject = nullptr) {
				renderStorage_->addMesh(component, robject);
			}
			BufferInterface* getElementBuffer() {
				return renderStorage_->buf();
			}
			void preframeSetup(const glm::mat4& viewMatrix) {
				renderStorage_->buf()->commit();
				initialise(viewMatrix);
			}
			void toggleWiremeshMode() {
				pipeline_->switchMode();
			}

			template<typename PC>
			const VkPushConstantRange setupPushConstantRange(VkShaderStageFlagBits stages) {
				auto pcr = createPushConstantRange<PC>(stages, pushConstantOffset_);
				pushConstantInfos_.push_back(pcr.second);
				pushConstantOffset_ += sizeof(PC);
				return pcr.first;
			}

			template<typename PC>
			static const std::pair<VkPushConstantRange, PushConstantInfo> createPushConstantRange(VkShaderStageFlagBits stages, uint32_t offset) {
				static_assert(sizeof(PC) <= MAX_PUSH_CONSTANT_SIZE, "Push constant size is potentially beyond the limit.");
				VkPushConstantRange pushConstantRange = {};
				pushConstantRange.size = sizeof(PC);
				pushConstantRange.offset = offset;
				pushConstantRange.stageFlags = stages;

				PushConstantInfo pinfo;
				pinfo.size = sizeof(PC);
				pinfo.stages = stages;
				pinfo.barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				pinfo.barrier.pNext = NULL;
				//pinfo.barrier.srcAccessMask
				pinfo.offset = offset;

				return { pushConstantRange, pinfo };
			}
		protected:
			/*template<typename V>
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, VkPipelineLayoutCreateInfo layoutInfo,
				const std::string& vertexShader, const std::string& fragmentShader, const std::string& tessCtrlShader, const std::string& tessEvalShader,
				RendererPipeline::PrimitiveType patchVertexCount, bool enableDepthTest = true, VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, 
				std::array<VkSpecializationInfo, 4>* specConstants = nullptr);
			template<typename V>
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, VkPipelineLayoutCreateInfo layoutInfo,
				const std::string& vertexShader, const std::string& fragmentShader, VkPrimitiveTopology topology, bool enableDepthTest = true, VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
				std::array<VkSpecializationInfo, 2>* specConstants = nullptr);
			template<typename V>
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, VkPipelineLayoutCreateInfo layoutInfo,
				const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, VkPrimitiveTopology topology, 
				bool enableDepthTest = true, VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, std::array<VkSpecializationInfo, 3>* specConstants = nullptr);*/

			template<typename V>
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages, 
				PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount = RendererPipeline::PrimitiveType::NONE);

			void beginFrame(VkCommandBuffer cmdBuffer);

			LogicDevice* logicDevice_;
			RendererPipeline* pipeline_;
			RenderStorage* renderStorage_;
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

		inline void RendererBase::recordCompute(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
		{
		}

		inline std::vector<VkWriteDescriptorSet> RendererBase::getDescriptorWrites(uint32_t frameIdx)
		{
			std::vector<VkWriteDescriptorSet> writes;
			for (auto& buf : storageBuffers_) {
				writes.push_back(buf->descriptorWrite(descriptorSets_[frameIdx]));
			}
			return writes;
		}

		/*template<typename V>
		inline void RendererBase::createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent,
			VkPipelineLayoutCreateInfo layoutInfo, const std::string& vertexShader, const std::string& fragmentShader, const std::string& tessCtrlShader, const std::string& tessEvalShader,
			RendererPipeline::PrimitiveType patchVertexCount, bool enableDepthTest, VkFrontFace frontFace, std::array<VkSpecializationInfo, 4>* specConstants)
		{
			auto bindingDesc = V::getBindDesc(0, VK_VERTEX_INPUT_RATE_VERTEX);
			auto attribDesc = V::getAttribDescs(0);
			auto p = RendererPipeline::makeVertexInputInfo<V>(bindingDesc, attribDesc);
			pipeline_ = new RendererPipeline(logicDevice, renderPass, swapChainExtent, layoutInfo, p, vertexShader, fragmentShader, tessCtrlShader, tessEvalShader,
				patchVertexCount, frontFace, enableDepthTest, specConstants);
		}

		template<typename V>
		inline void RendererBase::createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent,
			VkPipelineLayoutCreateInfo layoutInfo, const std::string& vertexShader, const std::string& fragmentShader, VkPrimitiveTopology topology, bool enableDepthTest, VkFrontFace frontFace,
			std::array<VkSpecializationInfo, 2>* specConstants)
		{
			auto bindingDesc = V::getBindDesc(0, VK_VERTEX_INPUT_RATE_VERTEX);
			auto attribDesc = V::getAttribDescs(0);
			auto p = RendererPipeline::makeVertexInputInfo<V>(bindingDesc, attribDesc);
			pipeline_ = new RendererPipeline(logicDevice, renderPass, swapChainExtent, layoutInfo, p, vertexShader, fragmentShader, 
				topology, frontFace, enableDepthTest, specConstants);
		}

		template<typename V>
		inline void RendererBase::createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, VkPipelineLayoutCreateInfo layoutInfo, const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, VkPrimitiveTopology topology, bool enableDepthTest, VkFrontFace frontFace, std::array<VkSpecializationInfo, 3> * specConstants)
		{
			auto bindingDesc = V::getBindDesc(0, VK_VERTEX_INPUT_RATE_VERTEX);
			auto attribDesc = V::getAttribDescs(0);
			auto p = RendererPipeline::makeVertexInputInfo<V>(bindingDesc, attribDesc);
			pipeline_ = new RendererPipeline(logicDevice, renderPass, swapChainExtent, layoutInfo, p, vertexShader, fragmentShader, geometryShader, 
				topology, frontFace, enableDepthTest, specConstants);
		}*/

		template<typename V>
		inline void RendererBase::createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages, 
			PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount)
		{
			auto bindingDesc = V::getBindDesc(0, VK_VERTEX_INPUT_RATE_VERTEX);
			auto attribDesc = V::getAttribDescs(0);
			pipelineCreateInfo.vertexInputInfo = RendererPipeline::makeVertexInputInfo<V>(bindingDesc, attribDesc);
			pipeline_ = new RendererPipeline(logicDevice, renderPass, layoutInfo, stages, pipelineCreateInfo, patchVertexCount);
		}

		inline void RendererBase::beginFrame(VkCommandBuffer cmdBuffer)
		{
			EXPECTS(pipeline_ != nullptr);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getPipeline());
		}
	}
}
