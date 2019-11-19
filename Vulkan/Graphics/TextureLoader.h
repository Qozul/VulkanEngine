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
			static unsigned char* getCPUImage(std::string name, int width, int height, int channels, int format);
			static void freeCPUImage(unsigned char* image);
		private:
			VkFormat convertToVkFormat(unsigned int oldFormat);

			const LogicDevice* logicDevice_;
			DeviceMemory* deviceMemory_;

			static const std::string kPath;
			static const std::string kExt;
		};
	}
}
