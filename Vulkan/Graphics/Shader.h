/// Author: Ralph Ridley
/// Date: 29/10/18
/// Purpose: To define the Shader class with necessary wrappings around
///          a shader module for one or more pipelines.
#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class Shader {
		public:
			Shader(const LogicDevice* logicDevice, const std::string& fileName);
			~Shader();

			VkPipelineShaderStageCreateInfo getCreateInfo(VkShaderStageFlagBits stageFlagBit, VkSpecializationInfo* specConstants = NULL) const noexcept;

			VkShaderModule getModule() const noexcept;

		private:
			void createModule(const std::string& fileName);

			std::vector<char> readSPIRV(const std::string& fileName);

			const LogicDevice* logicDevice_;
			VkShaderModule module_;

			static const std::string kPath;
			static const std::string kExt;
			static const char* kInsertionName;
		};
	}
}
