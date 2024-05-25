#pragma once

#include <functional>

#include "Header.hpp"

namespace EVK {

class Devices {
public:
	Devices(const char *applicationName, std::vector<const char *> requiredExtensions, std::function<VkSurfaceKHR (VkInstance)> surfaceCreationFunction, std::function<VkExtent2D ()> _getExtentFunction);
	
	Devices() = delete;
	Devices(const Devices &) = delete;
	Devices &operator=(const Devices &) = delete;
	Devices(Devices &&) = default;
	Devices &operator=(Devices &&) = default;
	~Devices();
	
	// Tools
	// -----
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, const VkImageTiling &tiling, const VkFormatFeatureFlags &features) const;
	VkFormat FindDepthFormat() const;
	SwapChainSupportDetails QuerySwapChainSupport() const;
	VkCommandBuffer BeginSingleTimeCommands() const;
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
	void FillExistingDeviceLocalBuffer(VkBuffer bufferHandle, void *data, const VkDeviceSize &size) const;
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subResourceRange) const;
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth=1) const;
	
	// Builders
	// -----
	VkShaderModule CreateShaderModule(const char *filename) const;
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VmaAllocation &allocation, VmaAllocationInfo *allocationInfoDst=nullptr) const;
	void CreateImage(const VkImageCreateInfo &imageCI, VkMemoryPropertyFlags properties, VkImage &image, VmaAllocation &allocation) const;
	VkImageView CreateImageView(const VkImageViewCreateInfo &imageViewCI) const;
	void CreateAndFillDeviceLocalBuffer(VkBuffer &bufferHandle, VmaAllocation &allocation, void *data, const VkDeviceSize &size, const VkBufferUsageFlags &usageFlags) const;
	
	// Getters
	// -----
	VkFormatProperties GetFormatProperties(const VkFormat &format) const {
		VkFormatProperties ret;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &ret);
		return ret;
	}
	const VkPhysicalDeviceProperties &GetPhysicalDeviceProperties() const { return physicalDeviceProperties; }
#ifdef MSAA
	const VkSampleCountFlagBits &GetMSAASamples() const { return msaaSamples; }
#endif
	VkExtent2D GetSurfaceExtent() const { return getExtentFunction(); }
	const VmaAllocator &GetAllocator() const { return allocator; }
	const VkDevice &GetLogicalDevice() const { return logicalDevice; }
	const QueueFamilyIndices &GetQueueFamilyIndices() const { return queueFamilyIndices; }
	const VkCommandPool &GetCommandPool() const { return commandPool; }
	const VkQueue &GraphicsQueue() const { return graphicsQueue; }
	const VkQueue &PresentQueue() const { return presentQueue; }
	const VkQueue &ComputeQueue() const { return computeQueue; }
	const VkSurfaceKHR &Surface() const { return surface; }
	
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
	VkCommandPool commandPool;
	
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
};


} // namespace EVK
