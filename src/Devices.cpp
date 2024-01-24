#include <Base.hpp>

#include <set>

static uint8_t *ReadFile(const char *filename, size_t &sizeOut){
	FILE *fptr = fopen(filename, "r");
	if(!fptr) throw std::runtime_error(std::string("failed to open file: ") + std::string(filename));
	fseek(fptr, 0, SEEK_END);
	sizeOut = (size_t)ftell(fptr);
	uint8_t *ret = (uint8_t *)malloc(sizeOut);
	fseek(fptr, 0, SEEK_SET);
	fread(ret, 1, sizeOut, fptr);
	fclose(fptr);
	return ret;
}

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}
#endif

namespace EVK {

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const std::vector<const char *> instanceExtensions = {};

#define QUEUE_FAMILIES_N 2
static QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice &device, const VkSurfaceKHR &surface){
	QueueFamilyIndices ret;
	
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	VkQueueFamilyProperties queueFamilies[queueFamilyCount];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
	
	for(uint32_t i=0; i<queueFamilyCount; i++){
		if((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) ret.graphicsAndComputeFamily = i;
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if(presentSupport) ret.presentFamily = i;
	}
	
	return ret;
}

static bool CheckDeviceExtensionSupport(const VkPhysicalDevice &device){
	{
		uint32_t instancedExtensionCount;
		vkEnumerateInstanceExtensionProperties(nullptr, &instancedExtensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(instancedExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &instancedExtensionCount, availableExtensions.data());
		
		std::set<std::string> requiredExtensions(instanceExtensions.begin(), instanceExtensions.end());
		for(VkExtensionProperties extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);
		
		if(!requiredExtensions.empty()) return false;
	}
	
	{
		uint32_t deviceExtensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, availableExtensions.data());
		
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for(VkExtensionProperties extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);
		
		if(!requiredExtensions.empty()) return false;
	}
	
	return true;
}
static SwapChainSupportDetails QueryDevicesSwapChainSupport(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) {
	SwapChainSupportDetails ret;
	
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &ret.capabilities);
	
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if(formatCount != 0){
		ret.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, ret.formats.data());
	}
	
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		ret.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, ret.presentModes.data());
	}
	
	return ret;
}
SwapChainSupportDetails Devices::QuerySwapChainSupport() const { return QueryDevicesSwapChainSupport(physicalDevice, surface); }

static bool IsDeviceSuitable(const VkPhysicalDevice &device, const VkSurfaceKHR &surface){
	QueueFamilyIndices indices = FindQueueFamilies(device, surface);
	
	bool swapChainAdequate = false;
	SwapChainSupportDetails swapChainSupport = QueryDevicesSwapChainSupport(device, surface);
	swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
	
	return indices.IsComplete() && CheckDeviceExtensionSupport(device) && swapChainAdequate
	&& supportedFeatures.samplerAnisotropy;
	// need to add all supported features here?
}

#ifndef NDEBUG
static VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo() {
	return {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = DebugCallback
	};
}
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
#endif

Devices::Devices(const char *applicationName,
				 std::vector<const char *> requiredExtensions,
				 std::function<VkSurfaceKHR (VkInstance)> surfaceCreationFunction,
				 std::function<VkExtent2D ()> _getExtentFunction)
: getExtentFunction(std::move(_getExtentFunction)) {
	
	// -----
	// Creating Vulkan instance
	// -----
	{
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = applicationName,
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "No Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VULKAN_VERSION_USING
		};
		
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#ifndef NDEBUG
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		createInfo.enabledExtensionCount = uint32_t(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo = GetDebugMessengerCreateInfo();
		createInfo.pNext = &debugCreateInfo;
		createInfo.enabledLayerCount = 0;
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
			throw std::runtime_error("failed to create devices.instance!");
	}
	
#ifndef NDEBUG
	// -----
	// Setting up the debug messenger
	// -----
	VkDebugUtilsMessengerCreateInfoEXT createInfo = GetDebugMessengerCreateInfo();
	if(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		throw std::runtime_error("failed to set up debug messenger!");
#endif
	
	surface = surfaceCreationFunction(instance);
	
	// -----
	// Picking physical graphics device
	// -----
	{
		physicalDevice = VK_NULL_HANDLE;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if(!deviceCount) throw std::runtime_error("no GPUs with Vulkan support!");
		VkPhysicalDevice physicalDevices[deviceCount];
		vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices);
		// this finds all GPUs with Vulkan support. If we are picky, we can look at the other supported features in each one and choose, but this is only needed for use of geometry shaders and things like that
		for(int i=0; i<deviceCount; i++){
			if(IsDeviceSuitable(physicalDevices[i], surface)){
				physicalDevice = physicalDevices[i];
#ifdef MSAA
				msaaSamples = GetMaxUsableSampleCount();
#endif
				break;
			}
		}
		if(physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("no suitable devices!");
		
		// ----- Getting properties -----
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	}
	
	
	// -----
	// Finding queue families for the chosen physical device
	// -----
	queueFamilyIndices = FindQueueFamilies(physicalDevice, surface);
	
	
	// -----
	// Creating logical device
	// -----
	{
		VkDeviceQueueCreateInfo queueCreateInfos[QUEUE_FAMILIES_N];
		uint32_t uniqueQueueFamilies[QUEUE_FAMILIES_N] = {
			queueFamilyIndices.graphicsAndComputeFamily.value(),
			queueFamilyIndices.presentFamily.value()
		};
		float queuePriority = 1.0f;
		for(int i=0; i<QUEUE_FAMILIES_N; i++){
			queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
			queueCreateInfos[i].queueCount = 1;
			queueCreateInfos[i].pQueuePriorities = &queuePriority;
		}
		
		// we specify features we'll be using here, like geometry shaders and anisotrophic filtering:
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		//deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE; // ? added to this to try fix textures, didn't help
		//deviceFeatures.shaderUniformBufferArrayDynamicIndexing = VK_TRUE; // ? added this because it looks like I should (dynamic ubos worked without it)
		
		VkDeviceCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = QUEUE_FAMILIES_N,
			.pQueueCreateInfos = queueCreateInfos,
			.enabledLayerCount = 0,
			.pEnabledFeatures = &deviceFeatures,
			.enabledExtensionCount = uint32_t(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data()
		};
		if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
			throw std::runtime_error("failed to create logical device!");
	}
	
	
	// -----
	// Getting the queue handles from the logical device
	// -----
	vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphicsAndComputeFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
	vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphicsAndComputeFamily.value(), 0, &computeQueue);
	
	
	// -----
	// Creating the memory allocator
	// -----
	{
		VmaVulkanFunctions vulkanFunctions = {
			.vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
			.vkGetDeviceProcAddr = &vkGetDeviceProcAddr
		};
		VmaAllocatorCreateInfo createInfo{
			.physicalDevice = physicalDevice,
			.device = logicalDevice,
			.pHeapSizeLimit = nullptr,
			.pVulkanFunctions = &vulkanFunctions,
			.instance = instance,
			.pTypeExternalMemoryHandleTypes = nullptr,
			.vulkanApiVersion = VULKAN_VERSION_USING
		};
		if(vmaCreateAllocator(&createInfo, &allocator) != VK_SUCCESS) throw std::runtime_error("failed to create memory devices.allocator!");
	}
}
Devices::~Devices(){
	vmaDestroyAllocator(allocator);
	vkDestroyDevice(logicalDevice, nullptr);
#ifndef NDEBUG
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

VkShaderModule Devices::CreateShaderModule(const char *filename) const {
	size_t size;
	uint8_t *bytes = ReadFile(filename, size);
	
	VkShaderModule ret;
	VkShaderModuleCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = (uint32_t *)bytes
	};
	if(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &ret) != VK_SUCCESS) throw std::runtime_error("failed to create shader module!");
	
	free(bytes);
	return ret;
}
#ifdef MSAA
VkSampleCountFlagBits Devices::GetMaxUsableSampleCount() const {
	VkPhysicalDeviceProperties deviceProperties = {};
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	VkSampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
	if(counts & VK_SAMPLE_COUNT_64_BIT){ return VK_SAMPLE_COUNT_64_BIT; }
	if(counts & VK_SAMPLE_COUNT_32_BIT){ return VK_SAMPLE_COUNT_32_BIT; }
	if(counts & VK_SAMPLE_COUNT_16_BIT){ return VK_SAMPLE_COUNT_16_BIT; }
	if(counts & VK_SAMPLE_COUNT_8_BIT){ return VK_SAMPLE_COUNT_8_BIT; }
	if(counts & VK_SAMPLE_COUNT_4_BIT){ return VK_SAMPLE_COUNT_4_BIT; }
	if(counts & VK_SAMPLE_COUNT_2_BIT){ return VK_SAMPLE_COUNT_2_BIT; }
	
	return VK_SAMPLE_COUNT_1_BIT;
}
#endif
uint32_t Devices::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for(uint32_t i=0; i<memProperties.memoryTypeCount; i++){
		if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}
VkFormat Devices::FindSupportedFormat(const VkFormat *candidates, int n, const VkImageTiling &tiling, const VkFormatFeatureFlags &features) const {
	for(int i=0; i<n; i++){
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, candidates[i], &props);
		
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return candidates[i];
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return candidates[i];
		}
	}
	
	throw std::runtime_error("failed to find supported format!");
}
VkFormat Devices::FindDepthFormat() const {
	const VkFormat candidates[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	return FindSupportedFormat(candidates,
							   3,
							   VK_IMAGE_TILING_OPTIMAL,
							   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace::EVK
