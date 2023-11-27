#include <Base.hpp>

#define VMA_IMPLEMENTATION // only put this define in one cpp file. put the below include in all cpp files that need it
#include <vma/vk_mem_alloc.h>

//#include <set>
#include <stdio.h>

namespace EVK {

VkCommandBuffer Interface::BeginSingleTimeCommands(){
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(devices.logicalDevice, &allocInfo, &commandBuffer);
	
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) throw std::runtime_error("failed to begin recording command buffer!");
	
	return commandBuffer;
}
void Interface::EndSingleTimeCommands(VkCommandBuffer commandBuffer){
	vkEndCommandBuffer(commandBuffer);
	
	// submitting the comand buffer to a queue that has 'VK_QUEUE_TRANSFER_BIT'. fortunately, this is case for any queue with 'VK_QUEUE_GRAPHICS_BIT' (or 'VK_QUEUE_COMPUTE_BIT')
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(devices.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(devices.graphicsQueue); // wait until commands have been executed before returning. using fences here could allow multiple transfers to be scheduled simultaneously
	
	vkFreeCommandBuffers(devices.logicalDevice, commandPool, 1, &commandBuffer);
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for(const auto& availableFormat : availableFormats){
		if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			return availableFormat;
		}
	}
	
	return availableFormats[0];
}
VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	
	return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window *const &window) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		SDL_GL_GetDrawableSize(window, &width, &height);
		
		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		
		std::cout << "Extent = " << actualExtent.width << ", " << actualExtent.height << "\n";
		
		return actualExtent;
	}
}
void Interface::CreateSwapChain(){
	SwapChainSupportDetails swapChainSupport = devices.QuerySwapChainSupport();
	
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, info.sdlWindowPtr);
	
	imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount){
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	
	{
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = devices.surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		
		uint32_t queueFamilyIndicesArray[] = {
			devices.queueFamilyIndices.graphicsAndComputeFamily.value(),
			devices.queueFamilyIndices.presentFamily.value()
		};
		
		if(devices.queueFamilyIndices.graphicsAndComputeFamily != devices.queueFamilyIndices.presentFamily){
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndicesArray;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}
		
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;
		
		if(vkCreateSwapchainKHR(devices.logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) throw std::runtime_error("failed to create swap chain!");
	}
	
	// Getting swap chain images
	vkGetSwapchainImagesKHR(devices.logicalDevice, swapChain, &imageCount, nullptr);
	swapChainImages = (VkImage *)malloc(imageCount*sizeof(VkImage));
	vkGetSwapchainImagesKHR(devices.logicalDevice, swapChain, &imageCount, swapChainImages);
	
	// Saving chosen format and extent of swap chain
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}
VkImageView Interface::CreateImageView(const VkImageViewCreateInfo &imageViewCI/*VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels*/){
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
	if(vkCreateImageView(devices.logicalDevice, &imageViewCI, nullptr, &ret) != VK_SUCCESS) throw std::runtime_error("failed to create texture image view!");
	return ret;
}
void Interface::CreateImage(const VkImageCreateInfo &imageCI, /*uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,*/ VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation& allocation) {
	
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
	 if(vkCreateImage(devices.logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) throw std::runtime_error("failed to create image!");
	 
	 VkMemoryRequirements memRequirements;
	 vkGetImageMemoryRequirements(devices.logicalDevice, image, &memRequirements);
	 VkMemoryAllocateInfo allocInfo{};
	 allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	 allocInfo.allocationSize = memRequirements.size;
	 allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	 if(vkAllocateMemory(devices.logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) throw std::runtime_error("failed to allocate image memory!");
	 // binding memory
	 vkBindImageMemory(devices.logicalDevice, image, imageMemory, 0);
	 */
	
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.requiredFlags = properties;
	allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocInfo.priority = 1.0f;
	if(vmaCreateImage(devices.allocator, &imageCI, &allocInfo, &image, &allocation, nullptr)) throw std::runtime_error("failed to create image!");
}
void Interface::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels){
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(devices.physicalDevice, imageFormat, &formatProperties);
	if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) throw std::runtime_error("texture image format does not support linear blitting!");
	
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
void Interface::CreateImageViews(){
	swapChainImageViews = (VkImageView *)malloc(imageCount*sizeof(VkImageView));
	for(size_t i=0; i<imageCount; i++){
		swapChainImageViews[i] = CreateImageView({
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			swapChainImages[i],
			VK_IMAGE_VIEW_TYPE_2D,
			swapChainImageFormat,
			{},
			{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		});
	}
}
#ifdef MSAA
void Interface::CreateColourResources() {
	VkFormat colorFormat = swapChainImageFormat;
	
	VkImageCreateInfo imageCI = {};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.extent.width = swapChainExtent.width;
	imageCI.extent.height = swapChainExtent.height;
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.format = colorFormat;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; // VK_IMAGE_TILING_LINEAR for row-major order if we want to access texels in the memory of the image
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	imageCI.samples = devices.msaaSamples;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colourImage, colourImageAllocation);
	
	colourImageView = CreateImageView({
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		colourImage,
		VK_IMAGE_VIEW_TYPE_2D,
		colorFormat,
		{},
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	});
}
#endif
void Interface::CreateDepthResources(){
	VkFormat depthFormat = devices.FindDepthFormat();
	
	VkImageCreateInfo imageCI = {};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.extent.width = swapChainExtent.width;
	imageCI.extent.height = swapChainExtent.height;
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.format = depthFormat;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; // VK_IMAGE_TILING_LINEAR for row-major order if we want to access texels in the memory of the image
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
#ifdef MSAA
	imageCI.samples = devices.msaaSamples;
#else
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
#endif
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageAllocation);
	
	depthImageView = CreateImageView({
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		depthImage,
		VK_IMAGE_VIEW_TYPE_2D,
		depthFormat,
		{},
		{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
	});
}
void Interface::CreateFramebuffers(){
	swapChainFramebuffers = (VkFramebuffer *)malloc(imageCount*sizeof(VkFramebuffer));
	
	for(size_t i=0; i<imageCount; i++){
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
#ifdef MSAA
		framebufferInfo.attachmentCount = 3;
		VkImageView attachments[3] = {
			colourImageView,
			depthImageView,
			swapChainImageViews[i]
		};
#else
		framebufferInfo.attachmentCount = 2;
		VkImageView attachments[2] = {
			swapChainImageViews[i],
			depthImageView
		};
#endif
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;
		
		if(vkCreateFramebuffer(devices.logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) throw std::runtime_error("failed to create framebuffer!");
	}
}


bool Interface::BuildBufferedRenderPass(int index, const BufferedRenderPassBlueprint &blueprint){
	BufferedRenderPass &ref = bufferedRenderPasses[index];
	ref = BufferedRenderPass();
	
	bool ret;
	if(!blueprint.width || !blueprint.height){
		ret = true;
		ref.width = swapChainExtent.width;
		ref.height = swapChainExtent.height;
	} else {
		ret = false;
		ref.width = blueprint.width;
		ref.height = blueprint.height;
	}
	
	if(vkCreateRenderPass(devices.logicalDevice, &blueprint.renderPassCI, nullptr, &ref.renderPass) != VK_SUCCESS) throw std::runtime_error("failed to create render pass!");
	
	VkFramebufferCreateInfo frameBufferCI = {};
	frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCI.renderPass = ref.renderPass;
	frameBufferCI.attachmentCount = blueprint.textureImagesN;
	VkImageView attachments[blueprint.textureImagesN];
	for(int i=0; i<blueprint.textureImagesN; i++) attachments[i] = textureImages[blueprint.targetTextureImageIndices[i]].view;
	frameBufferCI.pAttachments = attachments;
	frameBufferCI.width = ref.width;
	frameBufferCI.height = ref.height;
	frameBufferCI.layers = 1;
	for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) if(vkCreateFramebuffer(devices.logicalDevice, &frameBufferCI, nullptr, &ref.frameBuffersFlying[i]) != VK_SUCCESS) throw std::runtime_error("failed to create framebuffer!");
	
	return ret;
}
void Interface::BuildLayeredBufferedRenderPass(int index, const LayeredBufferedRenderPassBlueprint &blueprint){
	LayeredBufferedRenderPass &ref = layeredBufferedRenderPasses[index];
	ref = LayeredBufferedRenderPass(blueprint.layersN);
	
	ref.width = blueprint.width;
	ref.height = blueprint.height;
	
	if(vkCreateRenderPass(devices.logicalDevice, &blueprint.renderPassCI, nullptr, &ref.renderPass) != VK_SUCCESS) throw std::runtime_error("failed to create render pass!");
	
	for(int i=0; i<blueprint.layersN; i++){
		// Image view for this cascade's layer (inside the depth map)
		// This view is used to render to that specific depth image layer
		VkImageViewCreateInfo imageViewCI = {};
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		imageViewCI.format = blueprint.imageFormat;
		imageViewCI.subresourceRange.aspectMask = blueprint.imageAspectFlags;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = 1;
		imageViewCI.subresourceRange.baseArrayLayer = i;
		imageViewCI.subresourceRange.layerCount = 1;
		imageViewCI.image = textureImages[blueprint.targetTextureImageIndex].image;
		if(vkCreateImageView(devices.logicalDevice, &imageViewCI, nullptr, &ref.layerImageViews[i]) != VK_SUCCESS) throw std::runtime_error("failed to create image view!");
		
		VkFramebufferCreateInfo frameBufferCI = {};
		frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCI.renderPass = ref.renderPass;
		frameBufferCI.attachmentCount = 1;
		frameBufferCI.pAttachments = &ref.layerImageViews[i];
		frameBufferCI.width = blueprint.width;
		frameBufferCI.height = blueprint.height;
		frameBufferCI.layers = 1;
		for(int j=0; j<MAX_FRAMES_IN_FLIGHT; j++) if(vkCreateFramebuffer(devices.logicalDevice, &frameBufferCI, nullptr, &ref.frameBuffersFlying[MAX_FRAMES_IN_FLIGHT*i + j]) != VK_SUCCESS) throw std::runtime_error("failed to create framebuffer!");
	}
}

void Interface::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo *const &allocationInfoDst) {
	/*
	 VkBufferCreateInfo bufferInfo{};
	 // creating buffer
	 bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	 bufferInfo.size = size;
	 bufferInfo.usage = usage;
	 bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	 if(vkCreateBuffer(devices.logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) throw std::runtime_error("failed to create buffer!");
	 
	 // allocating memory for the buffer
	 VkMemoryRequirements memRequirements;
	 vkGetBufferMemoryRequirements(devices.logicalDevice, buffer, &memRequirements);
	 VkMemoryAllocateInfo allocInfo{};
	 allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	 allocInfo.allocationSize = memRequirements.size;
	 allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	 if(vkAllocateMemory(devices.logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) throw std::runtime_error("failed to allocate buffer memory!");
	 
	 vkBindBufferMemory(devices.logicalDevice, buffer, bufferMemory, 0);
	 */
	
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.requiredFlags = properties;
	if(allocationInfoDst) allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		VMA_ALLOCATION_CREATE_MAPPED_BIT;
	if(vmaCreateBuffer(devices.allocator, &bufferInfo, &allocInfo, &buffer, &allocation, allocationInfoDst) != VK_SUCCESS) throw std::runtime_error("failed to create buffer!");
}
void Interface::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	
	EndSingleTimeCommands(commandBuffer);
}
void Interface::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subResourceRange){
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
	} else throw std::invalid_argument("unsupported layout transition!");
	
	vkCmdPipelineBarrier(commandBuffer,
						 sourceStage, destinationStage,
						 0,
						 0, nullptr,
						 0, nullptr,
						 1, &barrier);
	
	EndSingleTimeCommands(commandBuffer);
}
void Interface::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {
		width,
		height,
		1
	};
	
	vkCmdCopyBufferToImage(commandBuffer,
						   buffer,
						   image,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1,
						   &region);
	
	EndSingleTimeCommands(commandBuffer);
}

void PNGImageBlueprint::Build(int index, Interface &interface){
	interface.BuildTextureImageFromFile(index, *this);
}
void CubemapPNGImageBlueprint::Build(int index, Interface &interface){
	interface.BuildCubemapImageFromFiles(index, *this);
}
void ManualImageBlueprint::Build(int index, Interface &interface){
	interface.BuildTextureImage(index, *this);
}
Interface::Interface(const InterfaceBlueprint &_info) : devices(_info.devices) {
	
	info.uniformBufferObjectsN = _info.uniformBufferObjectsN;
	info.storageBufferObjectsN = _info.storageBufferObjectsN;
	info.textureImagesN = _info.textureImagesN;
	info.textureSamplersN = _info.textureSamplersN;
	info.bufferedRenderPassesN = _info.bufferedRenderPassesN;
	info.layeredBufferedRenderPassesN = _info.layeredBufferedRenderPassesN;
	info.vertexBuffersN = _info.vertexBuffersN;
	info.indexBuffersN = _info.indexBuffersN;
	info.graphicsPipelinesN = _info.graphicsPipelinesN;
	info.computePipelinesN = _info.computePipelinesN;
	
	SDL_Event event;
	event.type = SDL_WINDOWEVENT;
	event.window.event = SDL_WINDOWEVENT_SIZE_CHANGED;
	ESDL::AddEventCallback((MemberFunction<Interface, void, SDL_Event>){this, &Interface::FramebufferResizeCallback}, event);
	
	
	// -----
	// Creating the command pool
	// -----
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = _info.devices.queueFamilyIndices.graphicsAndComputeFamily.value();
		if(vkCreateCommandPool(_info.devices.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) throw std::runtime_error("failed to create command pool!");
	}
	
	// -----
	// Allocating command buffers
	// -----
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
		if(vkAllocateCommandBuffers(_info.devices.logicalDevice, &allocInfo, commandBuffersFlying) != VK_SUCCESS) throw std::runtime_error("failed to allocate command buffers!");
	}
	
	
	
	// -----
	// Creating the swap chain, getting swap chain images and saving chosen format and extent of swap chain
	// -----
	CreateSwapChain();
	
	
	// -----
	// Creating swap chain image views
	// -----
	CreateImageViews();
	
	
	// -----
	// Creating the render pass
	// -----
	VkAttachmentDescription colourAttachment{};
	colourAttachment.format = swapChainImageFormat;
#ifdef MSAA
	colourAttachment.samples = _info.devices.msaaSamples;
#else
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
#endif
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference colourAttachmentRef{};
	colourAttachmentRef.attachment = 0;
#ifdef MSAA
	colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#else
	colourAttachmentRef.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
#endif
	
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = _info.devices.FindDepthFormat();
#ifdef MSAA
	depthAttachment.samples = _info.devices.msaaSamples;
#else
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
#endif
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	
#ifdef MSAA
	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = swapChainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;
#endif
	
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
#ifdef MSAA
	renderPassInfo.attachmentCount = 3;
	VkAttachmentDescription attachments[3] = {colourAttachment, depthAttachment, colorAttachmentResolve};
#else
	renderPassInfo.attachmentCount = 2;
	VkAttachmentDescription attachments[2] = {colourAttachment, depthAttachment};
#endif
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	if(vkCreateRenderPass(_info.devices.logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) throw std::runtime_error("failed to create render pass!");
	
	
#ifdef MSAA
	// -----
	// Creating the colour resources (for MSAA)
	// -----
	CreateColourResources();
#endif
	
	
	// -----
	// Creating the depth resources
	// -----
	CreateDepthResources();
	
	
	// -----
	// Allocating the vertex buffer arrays
	// -----
	vertexBuffersCreated = (bool *)malloc(_info.vertexBuffersN*sizeof(bool));
	vertexBufferHandles = (VkBuffer *)malloc(_info.vertexBuffersN*sizeof(VkBuffer));
	vertexBufferOffsets = (VkDeviceSize *)malloc(_info.vertexBuffersN*sizeof(VkDeviceSize));
	vertexBufferAllocations = (VmaAllocation *)malloc(_info.vertexBuffersN*sizeof(VmaAllocation));
	
	
	// -----
	// Allocating the index buffer arrays
	// -----
	indexBuffersCreated = (bool *)malloc(_info.indexBuffersN*sizeof(bool));
	indexBufferHandles = (VkBuffer *)malloc(_info.indexBuffersN*sizeof(VkBuffer));
	indexBufferOffsets = (VkDeviceSize *)malloc(_info.indexBuffersN*sizeof(VkDeviceSize));
	indexBufferAllocations = (VmaAllocation *)malloc(_info.indexBuffersN*sizeof(VmaAllocation));
	indexBufferCounts = (uint32_t *)malloc(_info.indexBuffersN*sizeof(uint32_t));
	
	
	// -----
	// Creating frame buffers
	// -----
	CreateFramebuffers();
	
	
	// -----
	// Allocating arrays and building structures
	// -----
	uniformBufferObjects = (UniformBufferObject *)malloc(_info.uniformBufferObjectsN*sizeof(UniformBufferObject));
	for(int i=0; i<_info.uniformBufferObjectsN; i++) BuildUBO(i, _info.uboBlueprints[i]);
	
	storageBufferObjects = (StorageBufferObject *)malloc(_info.storageBufferObjectsN*sizeof(StorageBufferObject));
	for(int i=0; i<_info.storageBufferObjectsN; i++) BuildSBO(i, _info.sboBlueprints[i]);
	
	textureSamplers = (VkSampler *)malloc(_info.textureSamplersN*sizeof(VkSampler));
	for(int i=0; i<_info.textureSamplersN; i++) BuildTextureSampler(i, _info.samplerBlueprints[i]);
	
	textureImages = (TextureImage *)malloc(_info.textureImagesN*sizeof(TextureImage));
	for(int i=0; i<_info.textureImagesN; i++) _info.imageBlueprintPtrs[i]->Build(i, *this);
	
	std::vector<int> resisingBRPIndices = {};
	std::vector<int> resisingImageIndices = {};
	bufferedRenderPasses = (BufferedRenderPass *)malloc(_info.bufferedRenderPassesN*sizeof(BufferedRenderPass));
	for(int i=0; i<_info.bufferedRenderPassesN; i++){
		if(BuildBufferedRenderPass(i, _info.bufferedRenderPassBlueprints[i])){
			resisingBRPIndices.push_back(i);
			for(int j=0; j<_info.bufferedRenderPassBlueprints[i].textureImagesN; j++){
				const int &ref = _info.bufferedRenderPassBlueprints[i].targetTextureImageIndices[j];
				if(std::find(resisingImageIndices.begin(), resisingImageIndices.end(), ref) == resisingImageIndices.end()) resisingImageIndices.push_back(ref);
			}
		}
	}
	resisingBRPsN = resisingBRPIndices.size();
	resisingBRPs = (ResisingBRP *)malloc(resisingBRPsN*sizeof(ResisingBRP));
	for(int i=0; i<resisingBRPsN; i++) resisingBRPs[i] = {resisingBRPIndices[i], _info.bufferedRenderPassBlueprints[resisingBRPIndices[i]]};
	resisingImagesN = resisingImageIndices.size();
	resisingImages = (ResisingImage *)malloc(resisingImagesN*sizeof(ResisingImage));
	for(int i=0; i<resisingImagesN; i++){
		resisingImages[i].index = resisingImageIndices[i];
		resisingImages[i].blueprint = *(ManualImageBlueprint *)(_info.imageBlueprintPtrs[resisingImageIndices[i]]);
	}
	
	layeredBufferedRenderPasses = (LayeredBufferedRenderPass *)malloc(_info.layeredBufferedRenderPassesN*sizeof(LayeredBufferedRenderPass));
	for(int i=0; i<_info.layeredBufferedRenderPassesN; i++) BuildLayeredBufferedRenderPass(i, _info.layeredBufferedRenderPassBlueprints[i]);
	
	graphicsPipelines = (GraphicsPipeline **)malloc(_info.graphicsPipelinesN*sizeof(GraphicsPipeline *));
	for(int i=0; i<_info.graphicsPipelinesN; i++) graphicsPipelines[i] = new GraphicsPipeline(*this, _info.graphicsPipelineBlueprints[i]);
	
	computePipelines = (ComputePipeline **)malloc(_info.computePipelinesN*sizeof(ComputePipeline *));
	for(int i=0; i<_info.computePipelinesN; i++) computePipelines[i] = new ComputePipeline(*this, _info.computePipelineBlueprints[i]);
	
	
	// no deleting the image blueprints after used by everything that needs them:
	for(int i=0; i<_info.textureImagesN; i++) delete _info.imageBlueprintPtrs[i];
	
	
	// -----
	// Creating sync objects
	// -----
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // starts the fence off as signalled so it doesn't wait indefinately upon first wait
		
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
			if (vkCreateSemaphore(_info.devices.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphoresFlying[i]) != VK_SUCCESS ||
				vkCreateSemaphore(_info.devices.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphoresFlying[i]) != VK_SUCCESS ||
				vkCreateFence(_info.devices.logicalDevice, &fenceInfo, nullptr, &inFlightFencesFlying[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create semaphores!");
			}
		}
	}
}
void Interface::RecreateResisingBRPsAndImages(){
	if(!resisingImagesN && !resisingBRPsN) return;
	
	for(int i=0; i<resisingBRPsN; i++){
		for(int j=0; j<MAX_FRAMES_IN_FLIGHT; j++) vkDestroyFramebuffer(devices.logicalDevice, bufferedRenderPasses[resisingBRPs[i].index].frameBuffersFlying[j], nullptr);
	}
	for(int i=0; i<resisingImagesN; i++){
		TextureImage &ref = textureImages[resisingImages[i].index];
		vkDestroyImageView(devices.logicalDevice, ref.view, nullptr);
		vmaDestroyImage(devices.allocator, ref.image, ref.allocation);
	}
	
	for(int i=0; i<resisingImagesN; i++){
		TextureImage &ref = textureImages[resisingImages[i].index];
		
		resisingImages[i].blueprint.imageCI.extent.width = swapChainExtent.width;
		resisingImages[i].blueprint.imageCI.extent.height = swapChainExtent.height;

		CreateImage(resisingImages[i].blueprint.imageCI, resisingImages[i].blueprint.properties, ref.image, ref.allocation);
		
		ref.view = CreateImageView({
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			ref.image,
			resisingImages[i].blueprint.imageViewType,
			resisingImages[i].blueprint.imageCI.format,
			{},
			{resisingImages[i].blueprint.aspectFlags, 0, resisingImages[i].blueprint.imageCI.mipLevels, 0, resisingImages[i].blueprint.imageCI.arrayLayers}
		});
	}
	for(int i=0; i<resisingBRPsN; i++){
		BufferedRenderPass &ref = bufferedRenderPasses[resisingBRPs[i].index];
		
		ref.width = swapChainExtent.width;
		ref.height = swapChainExtent.height;
		
		VkFramebufferCreateInfo frameBufferCI = {};
		frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCI.renderPass = ref.renderPass;
		frameBufferCI.attachmentCount = resisingBRPs[i].blueprint.textureImagesN;
		VkImageView attachments[resisingBRPs[i].blueprint.textureImagesN];
		for(int j=0; j<resisingBRPs[i].blueprint.textureImagesN; j++) attachments[j] = textureImages[resisingBRPs[i].blueprint.targetTextureImageIndices[j]].view;
		frameBufferCI.pAttachments = attachments;
		frameBufferCI.width = ref.width;
		frameBufferCI.height = ref.height;
		frameBufferCI.layers = 1;
		for(int j=0; j<MAX_FRAMES_IN_FLIGHT; j++) if(vkCreateFramebuffer(devices.logicalDevice, &frameBufferCI, nullptr, &ref.frameBuffersFlying[j]) != VK_SUCCESS) throw std::runtime_error("failed to create framebuffer!");
	}
	
	for(int i=0; i<info.graphicsPipelinesN; i++) graphicsPipelines[i]->UpdateDescriptorSets();
}
void Interface::CleanUpSwapChain(){
	vkDestroyImageView(devices.logicalDevice, depthImageView, nullptr);
	vmaDestroyImage(devices.allocator, depthImage, depthImageAllocation);
	
#ifdef MSAA
	vkDestroyImageView(devices.logicalDevice, colourImageView, nullptr);
	vmaDestroyImage(devices.allocator, colourImage, colourImageAllocation);
#endif
	
	vkDestroySwapchainKHR(devices.logicalDevice, swapChain, nullptr);
	for(size_t i=0; i<imageCount; i++) vkDestroyFramebuffer(devices.logicalDevice, swapChainFramebuffers[i], nullptr);
	for(size_t i=0; i<imageCount; i++) vkDestroyImageView(devices.logicalDevice, swapChainImageViews[i], nullptr);
	
	free(swapChainImages);
	free(swapChainImageViews);
	free(swapChainFramebuffers);
}
Interface::~Interface(){
	CleanUpSwapChain();
	
	for(int i=0; i<info.bufferedRenderPassesN; i++) bufferedRenderPasses[i].CleanUp(devices.logicalDevice);
	free(bufferedRenderPasses);
	for(int i=0; i<info.layeredBufferedRenderPassesN; i++) layeredBufferedRenderPasses[i].CleanUp(devices.logicalDevice);
	free(layeredBufferedRenderPasses);
	for(int i=0; i<info.textureSamplersN; i++) vkDestroySampler(devices.logicalDevice, textureSamplers[i], nullptr);
	free(textureSamplers);
	for(int i=0; i<info.textureImagesN; i++) textureImages[i].CleanUp(devices.logicalDevice, devices.allocator);
	free(textureImages);
	for(int i=0; i<info.uniformBufferObjectsN; i++) uniformBufferObjects[i].CleanUp(devices.allocator);
	free(uniformBufferObjects);
	for(int i=0; i<info.storageBufferObjectsN; i++) storageBufferObjects[i].CleanUp(devices.allocator);
	free(storageBufferObjects);
	for(int i=0; i<info.vertexBuffersN; i++) if(vertexBuffersCreated[i]){
		vmaDestroyBuffer(devices.allocator, vertexBufferHandles[i], vertexBufferAllocations[i]);
	}
	free(vertexBuffersCreated); free(vertexBufferHandles); free(vertexBufferOffsets); free(vertexBufferAllocations);
	for(int i=0; i<info.indexBuffersN; i++) if(indexBuffersCreated[i]){
		vmaDestroyBuffer(devices.allocator, indexBufferHandles[i], indexBufferAllocations[i]);
	}
	free(indexBuffersCreated); free(indexBufferHandles); free(indexBufferOffsets); free(indexBufferAllocations); free(indexBufferCounts);
	for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
		vkDestroySemaphore(devices.logicalDevice, imageAvailableSemaphoresFlying[i], nullptr);
		vkDestroySemaphore(devices.logicalDevice, renderFinishedSemaphoresFlying[i], nullptr);
		vkDestroyFence(devices.logicalDevice, inFlightFencesFlying[i], nullptr);
	}
	vmaDestroyAllocator(devices.allocator);
	for(int i=0; i<info.graphicsPipelinesN; i++) delete graphicsPipelines[i];
	free(graphicsPipelines);
	for(int i=0; i<info.computePipelinesN; i++) delete computePipelines[i];
		free(computePipelines);
	vkDestroyRenderPass(devices.logicalDevice, renderPass, nullptr);
	vkDestroyCommandPool(devices.logicalDevice, commandPool, nullptr);
	vkDestroySurfaceKHR(devices.instance, devices.surface, nullptr);
	vkDestroyDevice(devices.logicalDevice, nullptr);
	vkDestroyInstance(devices.instance, nullptr);
}
void Interface::FillDeviceLocalBuffer(VkBuffer &bufferHandle, VmaAllocation &allocation, void *const &data, const VkDeviceSize &size, const VkDeviceSize &offset, const VkBufferUsageFlags &usageFlags){
	// creating staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	
	memcpy(stagingAllocInfo.pMappedData, data, (size_t)size);
	vmaFlushAllocation(devices.allocator, stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// creating the new vertex buffer
	CreateBuffer(size, // size
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags, // usage
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // properties; device local means we generally can't use 'vkMapMemory', but it is quicker to access by the GPU
				 bufferHandle, // buffer handle output
				 allocation); // buffer memory handle output
	// copying the contents of the staging buffer into the vertex buffer
	CopyBuffer(stagingBuffer, bufferHandle, size);
	// cleaning up staging buffer
	vmaDestroyBuffer(devices.allocator, stagingBuffer, stagingAllocation);
}
void Interface::FillVertexBuffer(const int &vertexBufferIndex, void *const &vertices, const VkDeviceSize &size, const VkDeviceSize &offset){
	// destroying old vertex buffer if it exists
	if(vertexBuffersCreated[vertexBufferIndex]){
		vmaDestroyBuffer(devices.allocator, vertexBufferHandles[vertexBufferIndex], vertexBufferAllocations[vertexBufferIndex]);
	}
	
	FillDeviceLocalBuffer(vertexBufferHandles[vertexBufferIndex], vertexBufferAllocations[vertexBufferIndex], vertices, size, offset, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	
	vertexBufferOffsets[vertexBufferIndex] = offset;
	vertexBuffersCreated[vertexBufferIndex] = true;
}
void Interface::FillIndexBuffer(const int &indexBufferIndex, uint32_t *const &indices, const size_t &indexCount, const VkDeviceSize &offset){
	// destroying old index buffer if it exists
	if(indexBuffersCreated[indexBufferIndex]){
		vmaDestroyBuffer(devices.allocator, indexBufferHandles[indexBufferIndex], indexBufferAllocations[indexBufferIndex]);
	}
	
	FillDeviceLocalBuffer(indexBufferHandles[indexBufferIndex], indexBufferAllocations[indexBufferIndex], indices, (VkDeviceSize)(indexCount*sizeof(uint32_t)), offset, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	
	indexBufferOffsets[indexBufferIndex] = offset;
	indexBufferCounts[indexBufferIndex] = (uint32_t)indexCount;
	indexBuffersCreated[indexBufferIndex] = true;
}
bool Interface::BeginFrame(){
	// waiting until previous frame has finished rendering
	vkWaitForFences(devices.logicalDevice, 1, &inFlightFencesFlying[currentFrame], VK_TRUE, UINT64_MAX);
	
	// acquiring an image from the swap chain
	VkResult result = vkAcquireNextImageKHR(devices.logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphoresFlying[currentFrame], VK_NULL_HANDLE, &currentFrameImageIndex);
	
	// checking if we need to recreate the swap chain (e.g. if the window is resized)
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		RecreateSwapChain();
		//currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		return false;
	} else if(result != VK_SUCCESS) throw std::runtime_error("failed to acquire swap chain image!");
	
	// only reset the fence if we are submitting work
	vkResetFences(devices.logicalDevice, 1, &inFlightFencesFlying[currentFrame]);
	
	// recording the command buffer
	vkResetCommandBuffer(commandBuffersFlying[currentFrame], 0);
	
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Optional; this bit means we will submit this buffer once between each reset
	beginInfo.pInheritanceInfo = nullptr; // Optional
	if(vkBeginCommandBuffer(commandBuffersFlying[currentFrame], &beginInfo) != VK_SUCCESS) throw std::runtime_error("failed to begin recording command buffer!");
	
	return true;
}
void Interface::BeginFinalRenderPass(){
	VkClearValue clearValues[2] = {};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[currentFrameImageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;
	
	vkCmdBeginRenderPass(commandBuffersFlying[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
	
	// can filter at rasterizer stage to change rendered rectangle within viewport
	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
}
void Interface::EndFinalRenderPassAndFrame(){
	vkCmdEndRenderPass(commandBuffersFlying[currentFrame]);
	
	if(vkEndCommandBuffer(commandBuffersFlying[currentFrame]) != VK_SUCCESS) throw std::runtime_error("failed to record command buffer!");
	
	// submitting the command buffer to the graphics queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = {imageAvailableSemaphoresFlying[currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffersFlying[currentFrame];
	VkSemaphore signalSemaphores[] = {renderFinishedSemaphoresFlying[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	if(vkQueueSubmit(devices.graphicsQueue, 1, &submitInfo, inFlightFencesFlying[currentFrame]) != VK_SUCCESS) throw std::runtime_error("failed to submit draw command buffer!");
	
	// presenting on the present queue
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = {swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &currentFrameImageIndex;
	presentInfo.pResults = nullptr; // Optional
	vkQueuePresentKHR(devices.presentQueue, &presentInfo);
	
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
/*
 void Vulkan::CmdBeginRenderPass(const VkRenderPassBeginInfo *const &renderPassBeginInfo, const VkSubpassContents &subpassContents){
 vkCmdBeginRenderPass(commandBuffersFlying[currentFrame], renderPassBeginInfo, subpassContents);
 
 VkViewport viewport{};
 viewport.x = (float)renderPassBeginInfo->renderArea.offset.x;
 viewport.y = (float)renderPassBeginInfo->renderArea.offset.y;
 viewport.width = (float)renderPassBeginInfo->renderArea.extent.width;
 viewport.height = (float)renderPassBeginInfo->renderArea.extent.height;
 viewport.minDepth = 0.0f;
 viewport.maxDepth = 1.0f;
 vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
 
 // can filter at rasterizer stage to change rendered rectangle within viewport
 VkRect2D scissor{};
 scissor.offset = renderPassBeginInfo->renderArea.offset;
 scissor.extent = renderPassBeginInfo->renderArea.extent;
 vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
 }
 */
void Interface::CmdEndRenderPass(){
	vkCmdEndRenderPass(commandBuffersFlying[currentFrame]);
}

void Interface::CmdBindVertexBuffer(const uint32_t &binding, const int &index){
	vkCmdBindVertexBuffers(commandBuffersFlying[currentFrame], binding, 1, &vertexBufferHandles[index], &vertexBufferOffsets[index]);
}
void Interface::CmdBindIndexBuffer(const int &index, const VkIndexType &type){
	vkCmdBindIndexBuffer(commandBuffersFlying[currentFrame], indexBufferHandles[index], indexBufferOffsets[index], type);
}
void Interface::CmdDraw(const uint32_t &vertexCount, const uint32_t &instanceCount, const uint32_t &firstVertex, const uint32_t &firstInstance){
	vkCmdDraw(commandBuffersFlying[currentFrame], vertexCount, instanceCount, firstVertex, firstInstance);
}
void Interface::CmdDrawIndexed(const uint32_t &indexCount, const uint32_t &instanceCount, const uint32_t &firstIndex, const int32_t &vertexOffset, const uint32_t &firstInstance){
	vkCmdDrawIndexed(commandBuffersFlying[currentFrame], indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
void Interface::CmdSetDepthBias(const float &constantFactor, const float &clamp, const float &slopeFactor){
	vkCmdSetDepthBias(commandBuffersFlying[currentFrame], constantFactor, clamp, slopeFactor);
}
void Interface::CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ){
	vkCmdDispatch(commandBuffersFlying[currentFrame], groupCountX, groupCountY, groupCountZ);
}
void Interface::CmdBeginBufferedRenderPass(const int &bufferedRenderPassIndex, const VkSubpassContents &subpassContents, int clearValueCount, const VkClearValue *const &clearValues){
	BufferedRenderPass &ref = bufferedRenderPasses[bufferedRenderPassIndex];
	
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = ref.renderPass;
	renderPassBeginInfo.framebuffer = ref.frameBuffersFlying[currentFrame];
	renderPassBeginInfo.renderArea = {{0, 0}, {ref.width, ref.height}};
	renderPassBeginInfo.clearValueCount = clearValueCount;
	renderPassBeginInfo.pClearValues = clearValues;
	
	vkCmdBeginRenderPass(commandBuffersFlying[currentFrame], &renderPassBeginInfo, subpassContents);
	
	VkViewport viewport{};
	viewport.x = (float)renderPassBeginInfo.renderArea.offset.x;
	viewport.y = (float)renderPassBeginInfo.renderArea.offset.y;
	viewport.width = (float)renderPassBeginInfo.renderArea.extent.width;
	viewport.height = (float)renderPassBeginInfo.renderArea.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
	
	// can filter at rasterizer stage to change rendered rectangle within viewport
	VkRect2D scissor{};
	scissor.offset = renderPassBeginInfo.renderArea.offset;
	scissor.extent = renderPassBeginInfo.renderArea.extent;
	vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
}
void Interface::CmdBeginLayeredBufferedRenderPass(const int &layeredBufferedRenderPassIndex, const VkSubpassContents &subpassContents, int clearValueCount, const VkClearValue *const &clearValues, const int &layer){
	LayeredBufferedRenderPass &ref = layeredBufferedRenderPasses[layeredBufferedRenderPassIndex];
	
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = ref.renderPass;
	renderPassBeginInfo.framebuffer = ref.frameBuffersFlying[MAX_FRAMES_IN_FLIGHT*layer + currentFrame];
	renderPassBeginInfo.renderArea = {{0, 0}, {ref.width, ref.height}};
	renderPassBeginInfo.clearValueCount = clearValueCount;
	renderPassBeginInfo.pClearValues = clearValues;
	
	vkCmdBeginRenderPass(commandBuffersFlying[currentFrame], &renderPassBeginInfo, subpassContents);
	
	VkViewport viewport{};
	viewport.x = (float)renderPassBeginInfo.renderArea.offset.x;
	viewport.y = (float)renderPassBeginInfo.renderArea.offset.y;
	viewport.width = (float)renderPassBeginInfo.renderArea.extent.width;
	viewport.height = (float)renderPassBeginInfo.renderArea.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
	
	// can filter at rasterizer stage to change rendered rectangle within viewport
	VkRect2D scissor{};
	scissor.offset = renderPassBeginInfo.renderArea.offset;
	scissor.extent = renderPassBeginInfo.renderArea.extent;
	vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
}

void Interface::BuildUBO(int index, const UniformBufferObjectBlueprint &blueprint){
	UniformBufferObject &newUBORef = uniformBufferObjects[index];
	newUBORef = UniformBufferObject();
	
	newUBORef.dynamicRepeats = blueprint.dynamicRepeats;
	
	if(newUBORef.dynamicRepeats > 1){ // dynamic
		// Calculate required alignment based on minimum device offset alignment
		const VkDeviceSize minUboAlignment = devices.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
		newUBORef.dynamicAlignment = blueprint.size;
		if(minUboAlignment > 0) newUBORef.dynamicAlignment = (newUBORef.dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		newUBORef.size = newUBORef.dynamicAlignment*newUBORef.dynamicRepeats;
	} else { // not dynamic
		newUBORef.size = blueprint.size;
	}
	
	for(size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
		// creating buffer
		CreateBuffer(newUBORef.size,
					 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					 newUBORef.buffersFlying[i],
					 newUBORef.allocationsFlying[i],
					 &(newUBORef.allocationInfosFlying[i]));
	}
}
void Interface::BuildSBO(int index, const StorageBufferObjectBlueprint &blueprint){
	StorageBufferObject &newSBORef = storageBufferObjects[index];
	newSBORef = StorageBufferObject();
	
	newSBORef.size = blueprint.size;
	
	for(size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
		// creating buffer
		CreateBuffer(newSBORef.size,
					 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | blueprint.usages,
					 blueprint.memoryProperties,
					 newSBORef.buffersFlying[i],
					 newSBORef.allocationsFlying[i],
					 &(newSBORef.allocationInfosFlying[i]));
	}
}
void Interface::BuildTextureImage(int index, ManualImageBlueprint blueprint){
	TextureImage &ref = textureImages[index];
	ref = TextureImage();
	
	if(blueprint.imageCI.extent.width == 0 || blueprint.imageCI.extent.height == 0){
		blueprint.imageCI.extent.width = swapChainExtent.width;
		blueprint.imageCI.extent.height = swapChainExtent.height;
	}
	
	CreateImage(blueprint.imageCI, blueprint.properties, ref.image, ref.allocation);
	
	ref.view = CreateImageView({
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		ref.image,
		blueprint.imageViewType,
		blueprint.imageCI.format,
		{},
		{blueprint.aspectFlags, 0, blueprint.imageCI.mipLevels, 0, blueprint.imageCI.arrayLayers}
	});
}
void Interface::BuildTextureImageFromFile(int index, const PNGImageBlueprint &blueprint){
	TextureImage &ref = textureImages[index];
	ref = TextureImage();
	
	// loading image onto an sdl devices.surface
	SDL_Surface *const surface = IMG_Load(blueprint.imageFilename);
	if(!surface) throw std::runtime_error("failed to load texture image!");
	const uint32_t width = surface->w;
	const uint32_t height = surface->h;
	const VkDeviceSize imageSize = height*surface->pitch;
	const VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;//SDLPixelFormatToVulkanFormat((SDL_PixelFormatEnum)devices.surface->format->format); // 24 bit-depth images don't seem to work
	
	// calculated number of mipmap levels
	ref.mipLevels = (uint32_t)floor(log2((double)(width > height ? width : height))) + 1;
	
	// creating a staging buffer, copying in the pixels and then freeing the SDL devices.surface and the staging buffer memory allocation
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	memcpy(stagingAllocInfo.pMappedData, surface->pixels, (size_t)imageSize);
	SDL_FreeSurface(surface);
	vmaFlushAllocation(devices.allocator, stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// creating the image
	VkImageCreateInfo imageCI = {};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.extent.width = width;
	imageCI.extent.height = height;
	imageCI.extent.depth = 1;
	imageCI.mipLevels = ref.mipLevels;
	imageCI.arrayLayers = 1;
	imageCI.format = imageFormat;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL; // VK_IMAGE_TILING_LINEAR for row-major order if we want to access texels in the memory of the image
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ref.image, ref.allocation);
	
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = ref.mipLevels;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;
	TransitionImageLayout(ref.image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	CopyBufferToImage(stagingBuffer, ref.image, width, height);
	GenerateMipmaps(ref.image, imageFormat, width, height, ref.mipLevels);
	
	vmaDestroyBuffer(devices.allocator, stagingBuffer, stagingAllocation);
	
	// Creating an image view for the texture image
	ref.view = CreateImageView({
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		ref.image,
		VK_IMAGE_VIEW_TYPE_2D,
		imageFormat,
		{},
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, ref.mipLevels, 0, 1}
	});
}
void Interface::BuildCubemapImageFromFiles(int index, const CubemapPNGImageBlueprint &blueprint){
	TextureImage &ref = textureImages[index];
	ref = TextureImage();
	
	// loading image onto an sdl devices.surface
	SDL_Surface *surfaces[6];
	surfaces[0] = IMG_Load(blueprint.imageFilenames[0]);
	if(!surfaces[0]) throw std::runtime_error("failed to load texture image!");
	const uint32_t faceWidth = surfaces[0]->w;
	//const uint32_t width = 6*faceWidth;
	const uint32_t height = surfaces[0]->h;
	const size_t faceSize = height*surfaces[0]->pitch;
	const VkDeviceSize imageSize = 6*faceSize;
	const VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;//SDLPixelFormatToVulkanFormat((SDL_PixelFormatEnum)devices.surface->format->format); // 24 bit-depth images don't seem to work
	
	for(int i=1; i<6; i++){
		surfaces[i] = IMG_Load(blueprint.imageFilenames[i]);
		if(!surfaces[i]) throw std::runtime_error("failed to load texture image!");
#ifdef MAKE_ASSERTIONS
		assert(surfaces[i]->w == faceWidth);
		assert(surfaces[i]->h == height);
#endif
	}
	
	// creating a staging buffer, copying in the pixels and then freeing the SDL devices.surface and the staging buffer memory allocation
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	for(int i=0; i<6; i++){
		memcpy((uint8_t *)stagingAllocInfo.pMappedData + faceSize*i, surfaces[i]->pixels, faceSize);
		SDL_FreeSurface(surfaces[i]);
	}
	vmaFlushAllocation(devices.allocator, stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// creating the image
	VkImageCreateInfo imageCI = {};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.extent.width = faceWidth;
	imageCI.extent.height = height;
	imageCI.extent.depth = 1;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 6; // one for each cube face
	imageCI.format = imageFormat;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ref.image, ref.allocation);
	
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
	
	VkBufferImageCopy regions[6];
	for(int face=0; face<6; face++){
		regions[face].bufferOffset = faceSize*face;
		regions[face].bufferRowLength = 0;
		regions[face].bufferImageHeight = 0;
		regions[face].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions[face].imageSubresource.mipLevel = 0;
		regions[face].imageSubresource.baseArrayLayer = face;
		regions[face].imageSubresource.layerCount = 1;
		regions[face].imageOffset = {0, 0, 0};
		regions[face].imageExtent = {faceWidth, height, 1};
	}
	
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 6;
	TransitionImageLayout(ref.image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	
	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, ref.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, regions);
	
	TransitionImageLayout(ref.image, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
	
	EndSingleTimeCommands(commandBuffer);
	
	vmaDestroyBuffer(devices.allocator, stagingBuffer, stagingAllocation);
	
	// Creating an image view for the texture image
	ref.view = CreateImageView({
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		ref.image,
		VK_IMAGE_VIEW_TYPE_CUBE,
		imageFormat,
		{},
		{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6}
	});
}
void Interface::BuildTextureSampler(int index, const VkSamplerCreateInfo &samplerCI){
	if(vkCreateSampler(devices.logicalDevice, &samplerCI, nullptr, &textureSamplers[index]) != VK_SUCCESS) throw std::runtime_error("failed to create texture sampler!");
}


/*
 // VK_FORMAT_R8G8B8_SRGB and the other 24 bit-depth formats don't seem to work.
 VkFormat SDLPixelFormatToVulkanFormat(const SDL_PixelFormatEnum &sdlPixelFormat){
 switch(sdlPixelFormat){
 case SDL_PIXELFORMAT_RGB24: return VK_FORMAT_R8G8B8_SRGB;
 case SDL_PIXELFORMAT_RGBA32: return VK_FORMAT_R8G8B8A8_SRGB;
 default: {
 throw std::runtime_error("Unrecognised image format!");
 }
 }
 }
 */

} // namespace::EVK
