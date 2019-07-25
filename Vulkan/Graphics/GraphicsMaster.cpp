#include "GraphicsMaster.h""
#include "Validation.h"
#include "PhysicalDevice.h"
#include "LogicDevice.h"
#include "SwapChain.h"
#include "RendererBase.h"
#include "../System.h"
#include "../Assets/AssetManager.h"
#include "../Graphics/MeshLoader.h"

using namespace QZL;
using namespace QZL::Graphics;

constexpr auto kHoldConsole = false;

glm::mat4 GraphicsMaster::kProjectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);

EnvironmentArgs environmentArgs;

void GraphicsMaster::registerComponent(GraphicsComponent* component)
{
	auto renderer = component->getRendererType();
	renderers_[renderer]->registerComponent(component, masters_.assetManager->meshLoader->loadMesh(
		component->getMeshName(), *renderers_[renderer]->getElementBuffer(), component->getLoadFunc()));
}

void GraphicsMaster::setRenderer(RendererTypes type, RendererBase* renderer)
{
	renderers_[type] = renderer;
}

GraphicsMaster::GraphicsMaster(const SystemMasters& masters)
	: masters_(masters)
{
	kProjectionMatrix[1][1] *= -1;
	details_.master = this;
	environmentArgs.numObjectsX = 10;
	environmentArgs.numObjectsY = 10;
	environmentArgs.numObjectsZ = 10;
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
	for (auto ext : extensions)
		ASSERT(std::find(std::begin(availableExtNames), std::end(availableExtNames), ext) == availableExtNames.end());

	viewMatrix_ = new glm::mat4(glm::lookAt(glm::vec3(25.0f, 0.0f, 50.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));


	initInstance(extensions, enabledLayerCount, enabledLayerNames);
	CHECK_VKRESULT(glfwCreateWindowSurface(details_.instance, details_.window, nullptr, &details_.surface));

	validation_ = new Validation(details_.instance, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT);
	ASSERT(validation_ != nullptr);

	initDevices(enabledLayerCount, enabledLayerNames);
}

GraphicsMaster::~GraphicsMaster()
{
	SAFE_DELETE(details_.logicDevice);

	SAFE_DELETE(details_.physicalDevice);
	SAFE_DELETE(validation_);
	SAFE_DELETE(viewMatrix_);

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

void GraphicsMaster::initDevices(uint32_t& enabledLayerCount, const char* const*& enabledLayerNames)
{
	DeviceSurfaceCapabilities surfaceCapabilities;
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
	for (auto renderer : renderers_) {
		renderer.second->preframeSetup(*viewMatrix_);
	}
}

void GraphicsMaster::loop()
{
	details_.logicDevice->swapChain_->loop();
}
