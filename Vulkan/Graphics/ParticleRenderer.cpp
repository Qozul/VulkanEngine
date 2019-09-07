#include "ParticleRenderer.h"

using namespace QZL;
using namespace QZL::Graphics;

ParticleRenderer::ParticleRenderer(LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor, 
	const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const uint32_t particleSystemCount, const GlobalRenderData* globalRenderData)
	: RendererBase(logicDevice), descriptor_(descriptor)
{
}

QZL::Graphics::ParticleRenderer::~ParticleRenderer()
{
}

void QZL::Graphics::ParticleRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
}

void QZL::Graphics::ParticleRenderer::initialise(const glm::mat4& viewMatrix)
{
}
