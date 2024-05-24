#pragma once

#include "Header.hpp"

namespace EVK {

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsAndComputeFamily;
	std::optional<uint32_t> presentFamily;
	
	bool IsComplete(){
		return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
	}
};

class Devices {
public:
	Devices(const char *applicationName, std::vector<const char *> requiredExtensions, std::function<VkSurfaceKHR (VkInstance)> surfaceCreationFunction, std::function<VkExtent2D ()> _getExtentFunction);
	
	Devices() = delete;
	Devices(const Devices &) = delete;
	Devices &operator=(const Devices &) = delete;
	Devices(Devices &&) = default;
	Devices &operator=(Devices &&) = default;
	~Devices();
	
	
	// shader modules
	VkShaderModule CreateShaderModule(const char *filename) const;
	
	VkFormatProperties GetFormatProperties(const VkFormat &format) const {
		VkFormatProperties ret;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &ret);
		return ret;
	}
	
	const VkPhysicalDeviceProperties &GetPhysicalDeviceProperties() const { return physicalDeviceProperties; }
	
#ifdef MSAA
	const VkSampleCountFlagBits &GetMSAASamples() const { return msaaSamples; }
#endif
	
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, const VkImageTiling &tiling, const VkFormatFeatureFlags &features) const;
	VkFormat FindDepthFormat() const;
	
	SwapChainSupportDetails QuerySwapChainSupport() const;
	
	VkExtent2D GetSurfaceExtent() const { return getExtentFunction(); }
	
private:
	void CreateInstance(const char *applicationName, std::vector<const char *> requiredExtensions);
	void Init();
	
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	QueueFamilyIndices queueFamilyIndices;
	VkDevice logicalDevice;
	VmaAllocator allocator;
	
	VkDebugUtilsMessengerEXT debugMessenger;
	
	std::function<VkExtent2D ()> getExtentFunction;
	
	// queues:
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
	
#ifdef MSAA
	// multi-sampled anti-aliasing (MSAA):
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT; // default to 1 sample per pixel (no multi-sampling)
	VkSampleCountFlagBits GetMaxUsableSampleCount() const;
#endif
	
//	friend class Interface; ?
};


} // namespace EVK
