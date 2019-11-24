// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class Image;
		class LogicDevice;
		class DeviceMemory;

		class TextureLoader {
		public:
			TextureLoader(const LogicDevice* logicDevice);
			~TextureLoader();
			Image* loadTexture(const std::string& fileName, VkShaderStageFlags stages);
			Image* loadTextureGenerated(const std::string& fileName, VkShaderStageFlags stages, void* data, uint32_t width, uint32_t height, VkFormat format);
			Image* loadCubeTexture(const std::array<std::string, 6U> fileName, VkShaderStageFlags stages);
			static unsigned char* getCPUImage(std::string name, int width, int height, int channels, int format);
			static void freeCPUImage(unsigned char* image);
		private:
			VkFormat convertToVkFormat(unsigned int oldFormat);
			VkDeviceSize formatToSize(VkFormat format);

			const LogicDevice* logicDevice_;
			DeviceMemory* deviceMemory_;

			static const std::string kPath;
			static const std::string kExt;
		};
	}
}
