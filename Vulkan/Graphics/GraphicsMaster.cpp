// Author: Ralph Ridley
// Date: 01/11/19
#include "GraphicsMaster.h"
#include "GraphicsComponent.h"
#include "Validation.h"
#include "PhysicalDevice.h"
#include "LogicDevice.h"
#include "SwapChain.h"
#include "RendererBase.h"
#include "TextureManager.h"
#include "../SystemMasters.h"

using namespace QZL;
using namespace QZL::Graphics;

glm::mat4 GraphicsMaster::kProjectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, NEAR_PLANE_Z, FAR_PLANE_Z);

void GraphicsMaster::registerComponent(GraphicsComponent* component, RenderObject* robject, RendererTypes rtypeOverride)
{
	if (rtypeOverride == RendererTypes::kNone) {
		renderers_[component->getRendererType()]->registerComponent(component, robject);
	}
	else {
		renderers_[rtypeOverride]->registerComponent(component, robject);
	}
}

void GraphicsMaster::setRenderer(RendererTypes type, RendererBase* renderer)
{
	renderers_[type] = renderer;
}

ElementBufferObject* GraphicsMaster::getDynamicBuffer(RendererTypes type)
{
	return renderers_[type]->getElementBuffer();
}

LogicalCamera* GraphicsMaster::getLogicalCamera(RendererTypes type)
{
	return nullptr;
}

const bool GraphicsMaster::supportsOptionalExtension(OptionalExtensions ext)
{
	return details_.physicalDevice->optionalExtensionsEnabled_[ext];
}

GraphicsMaster::GraphicsMaster(SystemMasters& masters)
	: masters_(masters)
{
	kProjectionMatrix[1][1] *= -1;
	details_.master = this;
	std::vector<const char*> extensions;
	uint32_t enabledLayerCount;
	const char* const* enabledLayerNames;

	initGlfw(extensions);
	Validation::tryEnable(extensions, enabledLayerCount, enabledLayerNames);

	// Check the extensions are available
	auto availableExts = obtainVkData<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, "");
	std::vector<const char*> availableExtNames;
	std::transform(availableExts.begin(), availableExts.end(), std::back_inserter(availableExtNames),
		[](const VkExtensionProperties& prop) { return prop.extensionName; });
	for (auto ext : extensions) {
		ASSERT(std::find(std::begin(availableExtNames), std::end(availableExtNames), ext) == availableExtNames.end());
	}

	mainCamera_.viewMatrix = glm::mat4(glm::lookAt(glm::vec3(0.0f, 100.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
	mainCamera_.position = glm::vec3(0.0f, 10.0f, 0.0f);
	mainCamera_.lookPoint = glm::vec3(0.0f, 0.0f, 10.0f);

	initInstance(extensions, enabledLayerCount, enabledLayerNames);
	CHECK_VKRESULT(glfwCreateWindowSurface(details_.instance, details_.window, nullptr, &details_.surface));

	validation_ = new Validation(details_.instance, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT);
	ASSERT(validation_ != nullptr);

	DeviceSurfaceCapabilities surfaceCapabilities;
	initDevices(surfaceCapabilities, enabledLayerCount, enabledLayerNames);

	swapChain_ = new SwapChain(this, details_.window, details_.surface, details_.logicDevice, surfaceCapabilities);
	swapChain_->setCommandBuffers(std::vector<VkCommandBuffer>(details_.logicDevice->commandBuffers_.begin() + 1, details_.logicDevice->commandBuffers_.end()));
}

GraphicsMaster::~GraphicsMaster()
{
	masters_.inputManager->removeProfile("graphicsdebug");
	SAFE_DELETE(swapChain_);
	SAFE_DELETE(details_.logicDevice);

	SAFE_DELETE(details_.physicalDevice);
	SAFE_DELETE(validation_);

	vkDestroySurfaceKHR(details_.instance, details_.surface, nullptr);
	vkDestroyInstance(details_.instance, nullptr);

	glfwDestroyWindow(details_.window);
	glfwTerminate();
}

void GraphicsMaster::initGlfw(std::vector<const char*>& extensions)
{
	glfwInit();
	// Ensure glfw knows to ignore the openGL api and to not create a context for it
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	details_.window = glfwCreateWindow(kDefaultWidth, kDefaultHeight, "Vulkan Engine", nullptr, nullptr);
	
	ASSERT(details_.window != nullptr);
	
	glfwSetWindowUserPointer(details_.window, this);
	// TODO glfwSetFramebufferSizeCallback(details_.window, framebuffer_resize_callback);
	glfwSetInputMode(details_.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Add the glfw extensions to the required ones
	uint32_t extensionCount = 0;
	const char** glfwExts = glfwGetRequiredInstanceExtensions(&extensionCount);

	ASSERT(glfwExts != NULL);

	for (uint32_t i = 0; i < extensionCount; i++) {
		extensions.push_back(glfwExts[i]);
	}
}

void GraphicsMaster::initInstance(const std::vector<const char*>& extensions, uint32_t& enabledLayerCount, const char* const*& enabledLayerNames)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Engine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.flags = 0;
	createInfo.enabledLayerCount = enabledLayerCount;
	createInfo.ppEnabledLayerNames = enabledLayerNames;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	CHECK_VKRESULT(vkCreateInstance(&createInfo, nullptr, &details_.instance));
}

void GraphicsMaster::initDevices(DeviceSurfaceCapabilities& surfaceCapabilities, uint32_t& enabledLayerCount, const char* const*& enabledLayerNames)
{
	auto physicalDeviceHandles = obtainVkData<VkPhysicalDevice>(vkEnumeratePhysicalDevices, details_.instance);
	for (VkPhysicalDevice& handle : physicalDeviceHandles) {
		PhysicalDevice device(handle, details_.surface);
		if (device.isValid(surfaceCapabilities, details_.surface)) {
			details_.physicalDevice = new PhysicalDevice(device);
			details_.logicDevice = details_.physicalDevice->createLogicDevice(details_, surfaceCapabilities, enabledLayerCount, enabledLayerNames);
			return;
		}
	}
	throw std::runtime_error("Cannot create logic device");
}

void GraphicsMaster::preframeSetup()
{
	// Setup special input, all renderers should have been created by this point
	// NOTE that since the are renderers are stored in no order then it may not be the same key each time
	int i = 0;
	for (auto renderer : renderers_) {
		renderer.second->preframeSetup();
		inputProfile_.profileBindings.push_back({ { GLFW_KEY_1 + i }, std::bind(&RendererBase::toggleWiremeshMode, renderer.second), 1.0f });
		++i;
	}
	masters_.inputManager->addProfile("graphicsdebug", &inputProfile_);
}

void GraphicsMaster::loop()
{
	swapChain_->loop(mainCamera_);
}
