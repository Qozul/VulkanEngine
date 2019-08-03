#pragma once
#include "Mesh.h"
#include "RendererPipeline.h"
#include "StorageBuffer.h"
#include "RenderStorage.h"
#include "GraphicsComponent.h"
#include "ElementBuffer.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class Descriptor;

		struct ElementData {
			glm::mat4 modelMatrix;
			glm::mat4 mvpMatrix;
		};

		struct GlobalRenderData {
			Descriptor* globalDataDescriptor;
			size_t setIdx;
			VkDescriptorSetLayout layout;
		};

		class RendererBase {
		public:
			RendererBase(LogicDevice* logicDevice, DeviceMemory* deviceMemory)
				: pipeline_(nullptr), renderStorage_(new RenderStorage(deviceMemory)), logicDevice_(logicDevice) {
			}
			RendererBase(LogicDevice* logicDevice, RenderStorage* renderStorage)
				: pipeline_(nullptr), renderStorage_(renderStorage), logicDevice_(logicDevice) {
			}
			RendererBase(LogicDevice* logicDevice)
				: pipeline_(nullptr), renderStorage_(nullptr), logicDevice_(logicDevice) {
			}
			virtual ~RendererBase();
			virtual void recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer) = 0;
			virtual void recordCompute(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer);
			std::vector<VkWriteDescriptorSet> getDescriptorWrites(uint32_t frameIdx);
			virtual void initialise(const glm::mat4& viewMatrix) = 0;

			void registerComponent(GraphicsComponent* component, BasicMesh* mesh) {
				renderStorage_->addMesh(component, mesh);
			}
			ElementBuffer* getElementBuffer() {
				return renderStorage_->buf();
			}
			void preframeSetup(const glm::mat4& viewMatrix) {
				renderStorage_->buf()->commit();
				initialise(viewMatrix);
			}
			void toggleWiremeshMode() {
				pipeline_->switchMode();
			}
		protected:
			void createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, VkPipelineLayoutCreateInfo layoutInfo,
				const std::string& vertexShader, const std::string& fragmentShader, const std::string& tessCtrlShader = "", const std::string& tessEvalShader = "");
			void beginFrame(VkCommandBuffer cmdBuffer);

			LogicDevice* logicDevice_;
			RendererPipeline* pipeline_;
			RenderStorage* renderStorage_;
			std::vector<StorageBuffer*> storageBuffers_;
			std::vector<VkDescriptorSetLayout> pipelineLayouts_;
			std::vector<VkDescriptorSet> descriptorSets_;
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

		inline void RendererBase::createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent,
			VkPipelineLayoutCreateInfo layoutInfo, const std::string& vertexShader, const std::string& fragmentShader, const std::string& tessCtrlShader, const std::string& tessEvalShader)
		{
			if (tessCtrlShader == "" || tessEvalShader == "") {
				pipeline_ = new RendererPipeline(logicDevice, renderPass, swapChainExtent, layoutInfo, vertexShader, fragmentShader);
			}
			else {
				pipeline_ = new RendererPipeline(logicDevice, renderPass, swapChainExtent, layoutInfo, vertexShader, fragmentShader, tessCtrlShader, tessEvalShader);
			}
		}

		inline void RendererBase::beginFrame(VkCommandBuffer cmdBuffer)
		{
			EXPECTS(pipeline_ != nullptr);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getPipeline());
		}
	}
}
