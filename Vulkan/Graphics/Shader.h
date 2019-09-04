/// Author: Ralph Ridley
/// Date: 29/10/18
/// Purpose: To define the Shader class with necessary wrappings around
///          a shader module for one or more pipelines.
#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		class Shader {
		public:
			/// Caches the logic device and needs a name representing this shader.
			/// Shader should be stack allocated and destroyed at the end of the function it is used in
			/// Don't hold it longer as it needs to clean up its resources.
			/// Postcondition: module_ will be a valid module.
			Shader(const VkDevice logicDevice, const std::string& fileName);
			~Shader();
			/// Copy and move operations disallowed. Create it where you need it and let it go out of scope.
			Shader(const Shader& copy) = delete;
			Shader(Shader&& move) noexcept = delete;
			Shader& operator=(const Shader& copy) = delete;
			Shader operator=(Shader&& move_) noexcept = delete;

			/// Fills a create info struct and returns it for use in pipeline creation.
			VkPipelineShaderStageCreateInfo getCreateInfo(VkShaderStageFlagBits stageFlagBit, VkSpecializationInfo* specConstants = NULL) const noexcept;

			VkShaderModule getModule() const noexcept;

		private:
			/// Makes a shader module with the shader code. Possible i/o exceptions.
			/// Precondition (unchecked): fileName refers to an existing and valid spv file in the expected directory.
			void createModule(const std::string& fileName);

			/// Reads in a .spv found as kPath/filename/kExt to a byte buffer. Possible i/o exceptions.
			std::vector<char> readSPIRV(const std::string& fileName);

			const VkDevice logicDevice_;
			VkShaderModule module_;

			static const std::string kPath;
			static const std::string kExt;
			static const char* kInsertionName;
		};
	}
}
