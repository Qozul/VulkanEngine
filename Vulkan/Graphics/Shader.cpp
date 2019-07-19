/// Author: Ralph Ridley
/// Date: 29/10/18
/// Purpose: Definitions of Shader.h code
#include "Shader.h"

#include <stdlib.h> // system() for runBatchCompilation
#include <fstream> // for ifstream
#include <filesystem>

using namespace QZL;
using namespace QZL::Graphics;

const std::string Shader::kPath = "../Data/Shaders/SPIRV/";
const std::string Shader::kExt = ".spv";
const char* Shader::kInsertionName = "main";

Shader::Shader(VkDevice logicDevice, const std::string& fileName)
	: logicDevice_(logicDevice), module_(VK_NULL_HANDLE)
{
	createModule(fileName);
	ASSERT(module_ != VK_NULL_HANDLE); // Shader module creation failed.
}

Shader::~Shader()
{
	vkDestroyShaderModule(logicDevice_, module_, nullptr);
}

VkPipelineShaderStageCreateInfo Shader::getCreateInfo(VkShaderStageFlagBits stageFlagBit) const noexcept
{
	/* https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineShaderStageCreateInfo.html */
	VkPipelineShaderStageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.stage = stageFlagBit;
	createInfo.module = module_;
	createInfo.pName = kInsertionName;
	createInfo.pSpecializationInfo = NULL;
	return createInfo;
}

VkShaderModule Shader::getModule() const noexcept
{
	return module_;
}

void Shader::createModule(const std::string& fileName)
{
	std::vector<char> shaderCode = readSPIRV(fileName);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
	CHECK_VKRESULT(vkCreateShaderModule(logicDevice_, &createInfo, nullptr, &module_));
}


std::vector<char> Shader::readSPIRV(const std::string& fileName)
{
	std::ifstream file(kPath + fileName + kExt, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("Could not open file " + kPath + fileName + kExt);

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buf(fileSize);
	file.seekg(0);
	file.read(buf.data(), fileSize);
	file.close();

	return buf;
}
