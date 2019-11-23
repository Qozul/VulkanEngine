// Author: Ralph Ridley
// Date: 04/11/19
#include "SwapChain.h"
#include "LogicDevice.h"
#include "GeneralPass.h"
#include "DeferredPass.h"
#include "PostProcessPass.h"
#include "ShadowPass.h"
#include "LightingPass.h"
#include "RendererBase.h"
#include "GlobalRenderData.h"
#include "GraphicsMaster.h"
#include "TextureManager.h"
#include "SceneDescriptorInfo.h"
#include "../InputManager.h"
#include "../System.h"
#include "../Game/Scene.h"

#define MAX_FRAMES_IN_FLIGHT 2

using namespace QZL;
using namespace QZL::Graphics;

size_t SwapChain::numSwapChainImages = 0;

LogicalCamera* SwapChain::getCamera(size_t idx)
{
	return &frameInfo_.cameras[idx];
}

void SwapChain::loop()
{
	const uint32_t imgIdx = aquireImage();

	// Main scene light
	frameInfo_.sunHeight = glm::dot(glm::normalize(getCamera(1)->position), glm::vec3(0.0f, 1.0f, 0.0f));
	auto mainIntensity = glm::vec3(0.8f) * frameInfo_.sunHeight;
	Light lightingData = { frameInfo_.cameras[1].position, 2000.0f, glm::vec3(1.0), 0.01f };
	globalRenderData_->updateData(0, lightingData);

	auto commandLists = activeScene_->update(frameInfo_.cameras, NUM_CAMERAS, System::deltaTimeSeconds, imgIdx);

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores_[currentFrame_] };

	vkResetCommandBuffer(commandBuffers_[imgIdx], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	frameInfo_.cmdBuffer = commandBuffers_[imgIdx];
	frameInfo_.frameIdx = imgIdx;
	frameInfo_.commandLists = commandLists;
	frameInfo_.mainCameraIdx = 1;
	frameInfo_.viewportX = 0;

	CHECK_VKRESULT(vkBeginCommandBuffer(commandBuffers_[imgIdx], &beginInfo));

	// Shadow pass
	renderPasses_[0]->doFrame(frameInfo_);

	// Deferred geometry pass
	frameInfo_.mainCameraIdx = 0;
	renderPasses_[1]->doFrame(frameInfo_);
	// Lighting pass
	renderPasses_[2]->doFrame(frameInfo_);
	// Combine pass
	renderPasses_[3]->doFrame(frameInfo_);

	if (splitscreenEnabled_) {
		// Split screen geometry
		/*frameInfo_.viewportX = details_.extent.width / 2;
		frameInfo_.mainCameraIdx = 1;
		renderPasses_[0]->doFrame(frameInfo_);
		frameInfo_.viewportX = 0;
		frameInfo_.mainCameraIdx = 0;*/
	}

	// Post process ping ponging passes
	renderPasses_[4]->doFrame(frameInfo_);

	CHECK_VKRESULT(vkEndCommandBuffer(commandBuffers_[imgIdx]));

	submitQueue(imgIdx, signalSemaphores);

	present(imgIdx, signalSemaphores);
}

SwapChain::SwapChain(GraphicsMaster* master, GLFWwindow* window, VkSurfaceKHR surface, LogicDevice* logicDevice, DeviceSurfaceCapabilities& surfaceCapabilities)
	: logicDevice_(logicDevice), master_(master), splitscreenEnabled_(true)
{
	initSwapChain(window, surfaceCapabilities);
	initSwapChainImages(window, surface, surfaceCapabilities);
	numSwapChainImages = details_.images.size();
	initImageViews();
	initDepthFormat();
	globalRenderData_ = new GlobalRenderData(logicDevice, master->getMasters().textureManager, master->getMasters().textureManager->getSetlayoutBinding());
	createSyncObjects();
	toggleSplitscreen();

	frameInfo_.cameras[0] = {};
	frameInfo_.cameras[0].viewMatrix = glm::mat4(glm::lookAt(glm::vec3(0.0f, 100.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
	frameInfo_.cameras[0].projectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 1000.0f);
	frameInfo_.cameras[0].projectionMatrix[1][1] *= -1.0f;
	frameInfo_.cameras[0].position = glm::vec3(200.0f, 100.0f, 200.0f);
	frameInfo_.cameras[0].lookPoint = glm::vec3(0.0f, 0.0f, 10.0f);
	frameInfo_.cameras[1] = {};
	frameInfo_.cameras[1].position = glm::vec3(100.0f, 300.0f, 200.0f);
	frameInfo_.cameras[1].lookPoint = glm::vec3(100.0f, 10.0f, 300.0f);
	frameInfo_.cameras[1].viewMatrix = glm::lookAt(frameInfo_.cameras[1].position, frameInfo_.cameras[1].lookPoint, glm::vec3(0.0f, 1.0f, 0.0f));
	frameInfo_.cameras[1].projectionMatrix = glm::ortho(-400.0f, 400.0f, -400.0f, 400.0f, 200.0f, 1500.0f);
	frameInfo_.cameras[1].projectionMatrix[1][1] *= -1.0f;
}

SwapChain::~SwapChain()
{
	SAFE_DELETE(inputProfile_);
	SAFE_DELETE(globalRenderData_);
	SAFE_DELETE(computePrePass_);
	for (size_t i = 0; i < renderPasses_.size(); ++i) {
		SAFE_DELETE(renderPasses_[i]);
	}
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(*logicDevice_, renderFinishedSemaphores_[i], nullptr);
		vkDestroySemaphore(*logicDevice_, imageAvailableSemaphores_[i], nullptr);
		vkDestroyFence(*logicDevice_, inFlightFences_[i], nullptr);
	}
	for (auto view : details_.imageViews) {
		vkDestroyImageView(*logicDevice_, view, nullptr);
	}
	vkDestroySwapchainKHR(*logicDevice_, details_.swapChain, nullptr);
}

void SwapChain::initSwapChain(GLFWwindow* window, DeviceSurfaceCapabilities& surfaceCapabilities)
{
	details_.surfaceFormat = chooseFormat(surfaceCapabilities.formats);
	details_.presentMode = choosePresentMode(surfaceCapabilities.presentModes);
	details_.extent = chooseExtent(window, surfaceCapabilities.capabilities);
}

void SwapChain::initSwapChainImages(GLFWwindow* window, VkSurfaceKHR surface, DeviceSurfaceCapabilities& surfaceCapabilities)
{
	// Attempt to support triple buffering, request 3 images
	uint32_t imageCount = surfaceCapabilities.capabilities.minImageCount + 1;
	if (surfaceCapabilities.capabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.capabilities.maxImageCount)
		imageCount = surfaceCapabilities.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = details_.surfaceFormat.format;
	createInfo.imageColorSpace = details_.surfaceFormat.colorSpace;
	createInfo.imageExtent = details_.extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (logicDevice_->getFamilyIndex(QueueFamilyType::kGraphicsQueue) !=
		logicDevice_->getFamilyIndex(QueueFamilyType::kPresentationQueue)) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(QueueFamilyType::kNumQueueFamilyTypes);
		createInfo.pQueueFamilyIndices = logicDevice_->getAllIndices().data();
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}
	createInfo.preTransform = surfaceCapabilities.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = details_.presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE; // This NEEDS to be used if the swap chain has to be recreated

	CHECK_VKRESULT(vkCreateSwapchainKHR(*logicDevice_, &createInfo, nullptr, &details_.swapChain));

	details_.images = obtainVkData<VkImage>(vkGetSwapchainImagesKHR, *logicDevice_, details_.swapChain);
}

void SwapChain::initImageViews()
{
	// Need an image view for each image so that the image can be viewed
	details_.imageViews.resize(details_.images.size());
	for (auto i = 0; i < details_.images.size(); i++) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = details_.images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = details_.surfaceFormat.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		CHECK_VKRESULT(vkCreateImageView(*logicDevice_, &createInfo, nullptr, &details_.imageViews[i]));
	}
}

void SwapChain::initDepthFormat()
{
	details_.depthFormat = VkFormat::VK_FORMAT_UNDEFINED;
	for (VkFormat format : { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM }) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(logicDevice_->getPhysicalDevice(), format, &properties);
		if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT && properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
			details_.depthFormat = format;
			break;
		}
	}
	ASSERT(details_.depthFormat != VkFormat::VK_FORMAT_UNDEFINED);
}

VkSurfaceFormatKHR SwapChain::chooseFormat(std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	for (const auto& format : formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}
	return formats[0];
}

VkPresentModeKHR SwapChain::choosePresentMode(std::vector<VkPresentModeKHR>& presentModes)
{
	for (const auto& mode : presentModes) {
		// Mailbox for triple buffering is best
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
	}
	// Only mode guaranteed to be available
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseExtent(GLFWwindow* window, VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
		};
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void SwapChain::setCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers)
{
	commandBuffers_ = commandBuffers;
}

uint32_t SwapChain::aquireImage()
{
	vkWaitForFences(*logicDevice_, 1, &inFlightFences_[currentFrame_], VK_TRUE, std::numeric_limits<uint64_t>::max());

	uint32_t imgIdx;
	CHECK_VKRESULT(vkAcquireNextImageKHR(*logicDevice_, details_.swapChain, std::numeric_limits<uint64_t>::max(),
		imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imgIdx));
	return imgIdx;
}

void SwapChain::submitQueue(const uint32_t imgIdx, VkSemaphore signalSemaphores[])
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores_[currentFrame_] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers_[imgIdx];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(*logicDevice_, 1, &inFlightFences_[currentFrame_]);

	CHECK_VKRESULT(vkQueueSubmit(logicDevice_->getQueueHandle(QueueFamilyType::kGraphicsQueue), 1, &submitInfo, inFlightFences_[currentFrame_]));
}

void SwapChain::present(const uint32_t imgIdx, VkSemaphore signalSemaphores[])
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { details_.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imgIdx;

	CHECK_VKRESULT(vkQueuePresentKHR(logicDevice_->getQueueHandle(QueueFamilyType::kPresentationQueue), &presentInfo));

	currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void SwapChain::createSyncObjects() {
	imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		CHECK_VKRESULT(vkCreateSemaphore(*logicDevice_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]));
		CHECK_VKRESULT(vkCreateSemaphore(*logicDevice_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]));
		CHECK_VKRESULT(vkCreateFence(*logicDevice_, &fenceInfo, nullptr, &inFlightFences_[i]));
	}
}

void SwapChain::initialiseRenderPath(Scene* scene, SceneGraphicsInfo* graphicsInfo)
{

	inputProfile_ = new InputProfile();
	inputProfile_->profileBindings.push_back({ { GLFW_KEY_P }, std::bind(&SwapChain::toggleSplitscreen, this), 0.5f });
	master_->getMasters().inputManager->addProfile("SplitScreen", inputProfile_);

	activeScene_ = scene;
	renderPasses_.push_back(new ShadowPass(master_, logicDevice_, details_, globalRenderData_, graphicsInfo));
	renderPasses_.push_back(new DeferredPass(master_, logicDevice_, details_, globalRenderData_, graphicsInfo));
	renderPasses_.push_back(new LightingPass(master_, logicDevice_, details_, globalRenderData_, graphicsInfo));
	renderPasses_.push_back(new CombinePass(master_, logicDevice_, details_, globalRenderData_, graphicsInfo));
	renderPasses_.push_back(new PostProcessPass(master_, logicDevice_, details_, globalRenderData_, graphicsInfo));

	renderPasses_[0]->initRenderPassDependency({});
	renderPasses_[1]->initRenderPassDependency({});
	renderPasses_[2]->initRenderPassDependency({
		static_cast<DeferredPass*>(renderPasses_[1])->positionBuffer_, static_cast<DeferredPass*>(renderPasses_[1])->normalsBuffer_,
		static_cast<DeferredPass*>(renderPasses_[1])->depthBuffer_, static_cast<ShadowPass*>(renderPasses_[0])->depthBuffer_
	});
	renderPasses_[3]->initRenderPassDependency({
		static_cast<LightingPass*>(renderPasses_[2])->diffuseBuffer_, static_cast<LightingPass*>(renderPasses_[2])->specularBuffer_, 
		static_cast<DeferredPass*>(renderPasses_[1])->albedoBuffer_
	});
	renderPasses_[4]->initRenderPassDependency({ 
		static_cast<CombinePass*>(renderPasses_[3])->colourBuffer_, static_cast<DeferredPass*>(renderPasses_[1])->depthBuffer_
	});
}

void SwapChain::updateCameraAspectRatio()
{
	frameInfo_.cameras[0].projectionMatrix = !splitscreenEnabled_ ?
		glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 1000.0f) :
		glm::perspective(glm::radians(45.0f), 2.0f / 3.0f, 0.1f, 1000.0f);
	frameInfo_.cameras[0].projectionMatrix[1][1] *= -1.0f;
}

void SwapChain::toggleSplitscreen()
{
	splitscreenEnabled_ = !splitscreenEnabled_;
	frameInfo_.viewportWidth = splitscreenEnabled_ ? details_.extent.width / 2 : details_.extent.width;
	updateCameraAspectRatio();
}
