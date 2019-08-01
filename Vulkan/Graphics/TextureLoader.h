#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class Image2D;
		class LogicDevice;
		class DeviceMemory;

		class TextureLoader {
		public:
			TextureLoader(const LogicDevice* logicDevice);
			~TextureLoader();
			Image2D* loadTexture(const std::string& fileName);

		private:
			VkFormat convertToVkFormat(unsigned int oldFormat);

			const LogicDevice* logicDevice_;
			DeviceMemory* deviceMemory_;

			static const std::string kPath;
			static const std::string kExt;
		};
	}
}
