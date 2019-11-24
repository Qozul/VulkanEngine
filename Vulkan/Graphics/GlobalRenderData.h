// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"
#include "StorageBuffer.h"
#include "Descriptor.h"
#include "Light.h"
#include "LogicalCamera.h"

#define SSAO_KERNEL_SIZE 64

namespace QZL {
	namespace Graphics {
		class TextureSampler;
		class LogicDevice;

		struct PostProcessInfo {
			glm::vec2 ssaoNoiseScale;
			int ssaoKernelSize;
			float ssaoRadius;
			glm::vec4 ssaoSamples[SSAO_KERNEL_SIZE];
			float ssaoBias;
		};

		struct CameraInfo {
			glm::mat4 inverseViewProj;
			glm::mat4 viewMatrix;
			glm::mat4 projMatrix;
			glm::vec3 position;
			float nearPlaneZ;
			float farPlaneZ;
			float screenX;
			float screenY;
		};
		class GlobalRenderData {
			friend class SwapChain;
		public:
			VkDescriptorSet getSet() const {
				return set_;
			}
			VkDescriptorSetLayout getLayout() const {
				return layout_;
			}
			void updateData(uint32_t idx, Light& data);
			void updateCameraData(LogicalCamera& mainCamera, float screenX, float screenY);
			void updatePostData(float screenX, float screenY);
		private:
			GlobalRenderData(LogicDevice* logicDevice, TextureManager* textureManager, VkDescriptorSetLayoutBinding descriptorIndexBinding);
			~GlobalRenderData();
			void createDescriptorSet(LogicDevice* logicDevice, std::vector<VkDescriptorBindingFlagsEXT> bindingFlags, VkDescriptorSetLayoutBinding* descriptorIndexBinding = nullptr);

			VkDescriptorSet set_;
			VkDescriptorSetLayout layout_;
			DescriptorBuffer* lightingUbo_;
			DescriptorBuffer* cameraInfoUbo_;
			DescriptorBuffer* postProcessInfoUbo_;
			TextureSampler* environmentTexture_;
		};
	}
}
