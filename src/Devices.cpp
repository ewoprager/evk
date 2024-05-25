#include <set>
#include <fstream>
#include <iostream>

#include <vma/vk_mem_alloc.h>

#include <Devices.hpp>

static std::vector<char> ReadFile(const char *filename){
	std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
	std::ifstream::pos_type pos = ifs.tellg();

	if(!pos)
		return std::vector<char>{};

	std::vector<char> result(pos);

	ifs.seekg(0, std::ios::beg);
	ifs.read(result.data(), pos);

	return result;
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
	
	const bool extensionsSupported = CheckDeviceExtensionSupport(device);
	
	bool swapChainAdequate = false;
	if(extensionsSupported){
		SwapChainSupportDetails swapChainSupport = QueryDevicesSwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
	
	return indices.IsComplete() && extensionsSupported && swapChainAdequate
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
			throw std::runtime_error("failed to create instance!");
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
		if(vmaCreateAllocator(&createInfo, &allocator) != VK_SUCCESS)
			throw std::runtime_error("failed to create memory allocator!");
	}
	
	// -----
	// Creating the command pool
	// -----
	{
		const VkCommandPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value()
		};
		if(vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS){
			throw std::runtime_error("failed to create command pool!");
		}
	}
}
Devices::~Devices(){
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
	vmaDestroyAllocator(allocator);
	vkDestroyDevice(logicalDevice, nullptr);
#ifndef NDEBUG
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

VkCommandBuffer Devices::BeginSingleTimeCommands() const {
	const VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = commandPool,
		.commandBufferCount = 1
	};
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);
	
	const VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS){
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	
	return commandBuffer;
}
void Devices::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const {
	vkEndCommandBuffer(commandBuffer);
	
	// submitting the comand buffer to a queue that has 'VK_QUEUE_TRANSFER_BIT'. fortunately, this is case for any queue with 'VK_QUEUE_GRAPHICS_BIT' (or 'VK_QUEUE_COMPUTE_BIT')
	const VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue); // wait until commands have been executed before returning. using fences here could allow multiple transfers to be scheduled simultaneously
	
	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

VkShaderModule Devices::CreateShaderModule(const char *filename) const {
	const std::vector<char> bytes = ReadFile(filename);
	
	VkShaderModule ret;
	VkShaderModuleCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = uint32_t(bytes.size()),
		.pCode = (uint32_t *)(bytes.data())
	};
	if(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &ret) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");
	
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
VkFormat Devices::FindSupportedFormat(const std::vector<VkFormat> &candidates, const VkImageTiling &tiling, const VkFormatFeatureFlags &features) const {
	for(VkFormat format : candidates){
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
		
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	
	throw std::runtime_error("failed to find supported format!");
}
VkFormat Devices::FindDepthFormat() const {
	return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
							   VK_IMAGE_TILING_OPTIMAL,
							   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void Devices::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VmaAllocation &allocation, VmaAllocationInfo *allocationInfoDst) const {
	/*
	 VkBufferCreateInfo bufferInfo{};
	 // creating buffer
	 bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	 bufferInfo.size = size;
	 bufferInfo.usage = usage;
	 bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	 if(vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) throw std::runtime_error("failed to create buffer!");
	 
	 // allocating memory for the buffer
	 VkMemoryRequirements memRequirements;
	 vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);
	 VkMemoryAllocateInfo allocInfo{};
	 allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	 allocInfo.allocationSize = memRequirements.size;
	 allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	 if(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) throw std::runtime_error("failed to allocate buffer memory!");
	 
	 vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
	 */
	
	VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VmaAllocationCreateInfo allocInfo = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.requiredFlags = properties
	};
	if(allocationInfoDst){
		allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	if(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, allocationInfoDst) != VK_SUCCESS){
		throw std::runtime_error("failed to create buffer!");
	}
}

void Devices::CreateImage(const VkImageCreateInfo &imageCI, /*uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,*/ VkMemoryPropertyFlags properties, VkImage &image, VmaAllocation &allocation) const {
	
	/*
	 VkImageCreateInfo imageInfo{};
	 imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	 imageInfo.imageType = VK_IMAGE_TYPE_2D;
	 imageInfo.extent.width = width;
	 imageInfo.extent.height = height;
	 imageInfo.extent.depth = 1;
	 imageInfo.mipLevels = mipLevels;
	 imageInfo.arrayLayers = 1;
	 imageInfo.format = format;
	 imageInfo.tiling = tiling; // VK_IMAGE_TILING_LINEAR for row-major order if we want to access texels in the memory of the image
	 imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	 imageInfo.usage = usage;
	 imageInfo.samples = numSamples;
	 imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	 */
	/*
	 if(vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) throw std::runtime_error("failed to create image!");
	 
	 VkMemoryRequirements memRequirements;
	 vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);
	 VkMemoryAllocateInfo allocInfo{};
	 allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	 allocInfo.allocationSize = memRequirements.size;
	 allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	 if(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) throw std::runtime_error("failed to allocate image memory!");
	 // binding memory
	 vkBindImageMemory(logicalDevice, image, imageMemory, 0);
	 */
	
	VmaAllocationCreateInfo allocInfo = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.requiredFlags = properties,
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.priority = 1.0f
	};
	if(const VkResult res = vmaCreateImage(allocator, &imageCI, &allocInfo, &image, &allocation, nullptr);
	   res != VK_SUCCESS){
		throw std::runtime_error(std::string("failed to create image! VkResult = ") + std::to_string(res));
	}
}

VkImageView Devices::CreateImageView(const VkImageViewCreateInfo &imageViewCI/*VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels*/) const {
	/*
	 VkImageViewCreateInfo viewInfo{};
	 viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	 viewInfo.image = image;
	 viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	 viewInfo.format = format;
	 viewInfo.subresourceRange.aspectMask = aspectFlags;
	 viewInfo.subresourceRange.baseMipLevel = 0;
	 viewInfo.subresourceRange.levelCount = mipLevels;
	 viewInfo.subresourceRange.baseArrayLayer = 0;
	 viewInfo.subresourceRange.layerCount = 1;
	 */
	VkImageView ret;
	if(vkCreateImageView(logicalDevice, &imageViewCI, nullptr, &ret) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture image view!");
	return ret;
}

void Devices::CreateAndFillDeviceLocalBuffer(VkBuffer &bufferHandle, VmaAllocation &allocation, void *data, const VkDeviceSize &size, const VkBufferUsageFlags &usageFlags) const {
	// creating staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	
	memcpy(stagingAllocInfo.pMappedData, data, (size_t)size);
	vmaFlushAllocation(allocator, stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// creating the new vertex buffer
	CreateBuffer(size, // size
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags, // usage
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // properties; device local means we generally can't use 'vkMapMemory', but it is quicker to access by the GPU
				 bufferHandle, // buffer handle output
				 allocation); // buffer memory handle output
	// copying the contents of the staging buffer into the vertex buffer
	CopyBuffer(stagingBuffer, bufferHandle, size);
	// cleaning up staging buffer
	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void Devices::FillExistingDeviceLocalBuffer(VkBuffer bufferHandle, void *data, const VkDeviceSize &size) const {
	// creating staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	
	memcpy(stagingAllocInfo.pMappedData, data, (size_t)size);
	vmaFlushAllocation(allocator, stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// copying the contents of the staging buffer into the vertex buffer
	CopyBuffer(stagingBuffer, bufferHandle, size);
	// cleaning up staging buffer
	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void Devices::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const {
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
	if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)){
		throw std::runtime_error("texture image format does not support linear blitting!");
	}
	
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	
	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;
	for(uint32_t i=1; i<mipLevels; i++){
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);
		
		VkImageBlit blit{};
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		
		if(mipWidth > 1) mipWidth /= 2;
		if(mipHeight > 1) mipHeight /= 2;
		
		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = {mipWidth, mipHeight, 1};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		
		vkCmdBlitImage(commandBuffer,
					   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					   1, &blit,
					   VK_FILTER_LINEAR);
		
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);
	}
	
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer,
						 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						 0, nullptr,
						 0, nullptr,
						 1, &barrier);
	
	EndSingleTimeCommands(commandBuffer);
}

void Devices::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	
	EndSingleTimeCommands(commandBuffer);
}
void Devices::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subResourceRange) const {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = subResourceRange;
	
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else{
		throw std::invalid_argument("unsupported layout transition!");
	}
	
	vkCmdPipelineBarrier(commandBuffer,
						 sourceStage, destinationStage,
						 0,
						 0, nullptr,
						 0, nullptr,
						 1, &barrier);
	
	EndSingleTimeCommands(commandBuffer);
}
void Devices::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth) const {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	
	VkBufferImageCopy region{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.mipLevel = 0,
		.imageSubresource.baseArrayLayer = 0,
		.imageSubresource.layerCount = 1,
		.imageOffset = {0, 0, 0},
		.imageExtent = {width, height, depth}
	};
	
	vkCmdCopyBufferToImage(commandBuffer,
						   buffer,
						   image,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1,
						   &region);
	
	EndSingleTimeCommands(commandBuffer);
}

} // namespace::EVK
