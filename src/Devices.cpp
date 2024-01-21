#include <Base.hpp>

#include <set>

static uint8_t *ReadFile(const char *filename, size_t &sizeOut){
	FILE *fptr = fopen(filename, "r");
	if(!fptr) throw std::runtime_error("failed to open file!");
	fseek(fptr, 0, SEEK_END);
	sizeOut = (size_t)ftell(fptr);
	uint8_t *ret = (uint8_t *)malloc(sizeOut);
	fseek(fptr, 0, SEEK_SET);
	fread(ret, 1, sizeOut, fptr);
	fclose(fptr);
	return ret;
}

namespace EVK {

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
const std::vector<const char *> instanceExtensions = {};

#define QUEUE_FAMILIES_N 2
QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice &device, const VkSurfaceKHR &surface){
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

bool CheckDeviceExtensionSupport(const VkPhysicalDevice &device){
	{
		uint32_t instancedExtensionCount;
		vkEnumerateInstanceExtensionProperties(nullptr, &instancedExtensionCount, nullptr);
		VkExtensionProperties availableExtensions[instancedExtensionCount];
		vkEnumerateInstanceExtensionProperties(nullptr, &instancedExtensionCount, availableExtensions);
		
		std::set<std::string> requiredExtensions(instanceExtensions.begin(), instanceExtensions.end());
		for(int i=0; i<instancedExtensionCount; i++) requiredExtensions.erase(availableExtensions[i].extensionName);
		
		if(!requiredExtensions.empty()) return false;
	}
	
	{
		uint32_t deviceExtensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
		VkExtensionProperties availableExtensions[deviceExtensionCount];
		vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, availableExtensions);
		
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for(int i=0; i<deviceExtensionCount; i++) requiredExtensions.erase(availableExtensions[i].extensionName);
		
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

bool IsDeviceSuitable(const VkPhysicalDevice &device, const VkSurfaceKHR &surface){
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
			.apiVersion = VK_API_VERSION_1_3
		};
		
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		// extensions
//		SDL_Vulkan_GetInstanceExtensions(_sdlWindowPtr, &requiredExtensionCount, nullptr);
//		SDL_Vulkan_GetInstanceExtensions(_sdlWindowPtr, &requiredExtensionCount, requiredExtensions);
		requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		createInfo.enabledExtensionCount = uint32_t(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		
		//for(int i=0; i<requiredExtensionCount; i++) std::cout << requiredExtensions[i] << "\n";
		
		createInfo.enabledLayerCount = 0;
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		
		if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
			throw std::runtime_error("failed to create devices.instance!");
	}
	
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
		
		VkDeviceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pQueueCreateInfos = queueCreateInfos,
			.queueCreateInfoCount = QUEUE_FAMILIES_N,
			.pEnabledFeatures = &deviceFeatures,
			.enabledExtensionCount = (uint32_t)deviceExtensions.size(),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.enabledLayerCount = 0
		};
		if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) throw std::runtime_error("failed to create logical device!");
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
			.vulkanApiVersion = VK_API_VERSION_1_3
		};
		if(vmaCreateAllocator(&createInfo, &allocator) != VK_SUCCESS) throw std::runtime_error("failed to create memory devices.allocator!");
	}
}
Devices::~Devices(){
	vmaDestroyAllocator(allocator);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(logicalDevice, nullptr);
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
