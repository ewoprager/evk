#include <set>
#include <stdio.h>

#define VMA_IMPLEMENTATION // only put this define in one cpp file. put the below include in all cpp files that need it
#include <vma/vk_mem_alloc.h>

#include <Base.hpp>

namespace EVK {

VkCommandBuffer Interface::BeginSingleTimeCommands(){
	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = commandPool,
		.commandBufferCount = 1
	};
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(devices.logicalDevice, &allocInfo, &commandBuffer);
	
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording command buffer!");
	
	return commandBuffer;
}
void Interface::EndSingleTimeCommands(VkCommandBuffer commandBuffer){
	vkEndCommandBuffer(commandBuffer);
	
	// submitting the comand buffer to a queue that has 'VK_QUEUE_TRANSFER_BIT'. fortunately, this is case for any queue with 'VK_QUEUE_GRAPHICS_BIT' (or 'VK_QUEUE_COMPUTE_BIT')
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};
	vkQueueSubmit(devices.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(devices.graphicsQueue); // wait until commands have been executed before returning. using fences here could allow multiple transfers to be scheduled simultaneously
	
	vkFreeCommandBuffers(devices.logicalDevice, commandPool, 1, &commandBuffer);
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
	for(const auto& availableFormat : availableFormats){
		if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			return availableFormat;
		}
	}
	
	return availableFormats[0];
}
VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			return availablePresentMode;
		}
	}
	
	return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D actualExtent) {
//	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
//		return capabilities.currentExtent;
//	} else {
		
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		
		std::cout << "Extent = " << actualExtent.width << ", " << actualExtent.height << "\n";
		
		return actualExtent;
//	}
}
void Interface::CreateSwapChain(const VkExtent2D &actualExtent){
	SwapChainSupportDetails swapChainSupport = devices.QuerySwapChainSupport();
	
	const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	const VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, actualExtent);
	
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
		
		if(vkCreateSwapchainKHR(devices.logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
			throw std::runtime_error("failed to create swap chain!");
	}
	
	// Getting swap chain images
	vkGetSwapchainImagesKHR(devices.logicalDevice, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(devices.logicalDevice, swapChain, &imageCount, swapChainImages.data());
	
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
	if(vkCreateImageView(devices.logicalDevice, &imageViewCI, nullptr, &ret) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture image view!");
	return ret;
}
void Interface::CreateImage(const VkImageCreateInfo &imageCI, /*uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,*/ VkMemoryPropertyFlags properties, VkImage &image, VmaAllocation &allocation) {
	
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
	
	VmaAllocationCreateInfo allocInfo = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.requiredFlags = properties,
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.priority = 1.0f
	};
	if(const VkResult res = vmaCreateImage(devices.allocator, &imageCI, &allocInfo, &image, &allocation, nullptr);
	   res != VK_SUCCESS)
		throw std::runtime_error(std::string("failed to create image! VkResult = ") + std::to_string(res));
}
void Interface::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels){
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(devices.physicalDevice, imageFormat, &formatProperties);
	if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("texture image format does not support linear blitting!");
	
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
	swapChainImageViews.resize(imageCount);
	for(size_t i=0; i<imageCount; i++){
		swapChainImageViews[i] = CreateImageView({
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = swapChainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapChainImageFormat,
			.components = {},
			.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		});
	}
}
#ifdef MSAA
void Interface::CreateColourResources() {
	VkFormat colorFormat = swapChainImageFormat;
	
	VkImageCreateInfo imageCI = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = swapChainExtent.width,
		.extent.height = swapChainExtent.height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = colorFormat,
		.tiling = VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_TILING_LINEAR for row-major order if we want to access texels in the memory of the image
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.samples = devices.msaaSamples,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colourImage, colourImageAllocation);
	
	colourImageView = CreateImageView({
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = colourImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = colorFormat,
		.components = {},
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	});
}
#endif
void Interface::CreateDepthResources(){
	VkFormat depthFormat = devices.FindDepthFormat();
	
	VkImageCreateInfo imageCI = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent = {
			.width = swapChainExtent.width,
			.height = swapChainExtent.height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = depthFormat,
		.tiling = VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_TILING_LINEAR for row-major order if we want to access texels in the memory of the image
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
#ifdef MSAA
		.samples = devices.msaaSamples,
#else
		.samples = VK_SAMPLE_COUNT_1_BIT,
#endif
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageAllocation);
	
	depthImageView = CreateImageView({
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = depthImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = depthFormat,
		.components = {},
		.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}
	});
}
void Interface::CreateFramebuffers(){
	swapChainFramebuffers.resize(imageCount);
	
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


void Interface::BuildBufferedRenderPass(int index, const BufferedRenderPassBlueprint &blueprint){
	std::optional<BufferedRenderPass> &ref = bufferedRenderPasses[index];
	if(ref){
		ref->CleanUp(devices.logicalDevice);
	}
	ref = BufferedRenderPass();
	
	if(!blueprint.width || !blueprint.height){
		ref->resising = true;
		ref->width = swapChainExtent.width;
		ref->height = swapChainExtent.height;
	} else {
		ref->resising = false;
		ref->width = blueprint.width;
		ref->height = blueprint.height;
	}
	
	if(vkCreateRenderPass(devices.logicalDevice, &blueprint.renderPassCI, nullptr, &ref->renderPass) != VK_SUCCESS) throw std::runtime_error("failed to create render pass!");
	
	ref->targetTextureImageIndices = blueprint.targetTextureImageIndices;
}

void Interface::UpdateBufferedRenderPass(int index){
	std::optional<BufferedRenderPass> &ref = bufferedRenderPasses[index];
	if(!ref){
		throw std::runtime_error("Can't update buffered render pass as it hasn't been built.\n");
		return;
	}
	
	for(int imageIndex : ref->targetTextureImageIndices){
		if(!textureImages[imageIndex]){
			throw std::runtime_error("Can't update buffered render pass as not all its images have been built.\n");
			return;
		}
	}
	
	VkFramebufferCreateInfo frameBufferCI = {};
	frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCI.renderPass = ref->renderPass;
	frameBufferCI.attachmentCount = uint32_t(ref->targetTextureImageIndices.size());
	VkImageView attachments[ref->targetTextureImageIndices.size()];
	for(int i=0; i<ref->targetTextureImageIndices.size(); i++){
		if(!textureImages[ref->targetTextureImageIndices[i]])
			throw std::runtime_error("Buffered render pass target texture doesn't exist");
		attachments[i] = textureImages[ref->targetTextureImageIndices[i]]->view;
	}
	frameBufferCI.pAttachments = attachments;
	frameBufferCI.width = ref->width;
	frameBufferCI.height = ref->height;
	frameBufferCI.layers = 1;
	for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) if(vkCreateFramebuffer(devices.logicalDevice, &frameBufferCI, nullptr, &ref->frameBuffersFlying[i]) != VK_SUCCESS)
		throw std::runtime_error("failed to create framebuffer!");
	
	if(ref->resising){
		bool foundBRP = false;
		for(int &resisingBRP : resisingBRPs){
			if(resisingBRP == index){
				foundBRP = true;
				break;
			}
		}
		if(!foundBRP){
			resisingBRPs.push_back(index);
		}
		
		for(const int textureImageIndex : ref->targetTextureImageIndices){
			bool foundImage = false;
			for(int resisingImage : resisingImages){
				if(resisingImage == textureImageIndex){
					foundImage = true;
					break;
				}
			}
			if(!foundImage){
				resisingImages.push_back(textureImageIndex);
			}
		}
	}
}
void Interface::BuildLayeredBufferedRenderPass(int index, const LayeredBufferedRenderPassBlueprint &blueprint){
	std::optional<LayeredBufferedRenderPass> &ref = layeredBufferedRenderPasses[index];
	if(ref){
		ref->CleanUp(devices.logicalDevice);
	}
	ref = LayeredBufferedRenderPass(blueprint.layersN);
	
	ref->width = blueprint.width;
	ref->height = blueprint.height;
	
	if(vkCreateRenderPass(devices.logicalDevice, &blueprint.renderPassCI, nullptr, &ref->renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
	
	ref->layers.resize(blueprint.layersN);
	ref->targetTextureImageIndex = blueprint.targetTextureImageIndex;
	ref->imageFormat = blueprint.imageFormat;
	ref->imageAspectFlags = blueprint.imageAspectFlags;
}

void Interface::UpdateLayeredBufferedRenderPass(int index){
	std::optional<LayeredBufferedRenderPass> &ref = layeredBufferedRenderPasses[index];
	if(!ref){
		throw std::runtime_error("Can't update layered buffered render pass as it hasn't been built.\n");
		return;
	}
	
	if(!textureImages[ref->targetTextureImageIndex]){
		throw std::runtime_error("Can't update layered buffered render pass as its image hasn't been built.\n");
		return;
	}
	
	for(uint32_t i=0; i<ref->layers.size(); i++){
		// Image view for this cascade's layer (inside the depth map)
		// This view is used to render to that specific depth image layer
		VkImageViewCreateInfo imageViewCI = {
			.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			.format = ref->imageFormat,
			.subresourceRange.aspectMask = ref->imageAspectFlags,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = i,
			.subresourceRange.layerCount = 1,
			.image = textureImages[ref->targetTextureImageIndex]->image
		};
		if(vkCreateImageView(devices.logicalDevice, &imageViewCI, nullptr, &ref->layers[i].imageView) != VK_SUCCESS)
			throw std::runtime_error("failed to create image view!");
		
		VkFramebufferCreateInfo frameBufferCI = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = ref->renderPass,
			.attachmentCount = 1,
			.pAttachments = &ref->layers[i].imageView,
			.width = ref->width,
			.height = ref->height,
			.layers = 1
		};
		for(int j=0; j<MAX_FRAMES_IN_FLIGHT; j++) if(vkCreateFramebuffer(devices.logicalDevice, &frameBufferCI, nullptr, &ref->layers[i].frameBuffersFlying[j]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}
}

void Interface::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VmaAllocation &allocation, VmaAllocationInfo *allocationInfoDst) {
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
	
	VkBufferImageCopy region{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.mipLevel = 0,
		.imageSubresource.baseArrayLayer = 0,
		.imageSubresource.layerCount = 1,
		.imageOffset = {0, 0, 0},
		.imageExtent = {width, height, 1}
	};
	
	vkCmdCopyBufferToImage(commandBuffer,
						   buffer,
						   image,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1,
						   &region);
	
	EndSingleTimeCommands(commandBuffer);
}

Interface::Interface(const InterfaceBlueprint &blueprint) : devices(blueprint.devices) {
	
	// Initialising SDL_image
	IMG_Init(IMG_INIT_PNG);
	
	// -----
	// Creating the command pool
	// -----
	{
		VkCommandPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = blueprint.devices.queueFamilyIndices.graphicsAndComputeFamily.value()
		};
		if(vkCreateCommandPool(blueprint.devices.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
			throw std::runtime_error("failed to create command pool!");
	}
	
	// -----
	// Allocating command buffers
	// -----
	{
		VkCommandBufferAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = MAX_FRAMES_IN_FLIGHT
		};
		if(vkAllocateCommandBuffers(blueprint.devices.logicalDevice, &allocInfo, commandBuffersFlying) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate command buffers!");
		if(vkAllocateCommandBuffers(blueprint.devices.logicalDevice, &allocInfo, computeCommandBuffersFlying) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate compute command buffers!");
	}
	
	
	
	// -----
	// Creating the swap chain, getting swap chain images and saving chosen format and extent of swap chain
	// -----
	CreateSwapChain(devices.GetSurfaceExtent());
	
	
	// -----
	// Creating swap chain image views
	// -----
	CreateImageViews();
	
	
	// -----
	// Creating the render pass
	// -----
	VkAttachmentDescription colourAttachment{
		.format = swapChainImageFormat,
#ifdef MSAA
		.samples = blueprint.devices.msaaSamples,
#else
		.samples = VK_SAMPLE_COUNT_1_BIT,
#endif
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
#ifdef MSAA
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
#else
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
#endif
	};
	VkAttachmentReference colourAttachmentRef{
		.attachment = 0,
#ifdef MSAA
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
#else
		.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
#endif
	};
	
	VkAttachmentDescription depthAttachment{
		.format = blueprint.devices.FindDepthFormat(),
#ifdef MSAA
		.samples = blueprint.devices.msaaSamples,
#else
		.samples = VK_SAMPLE_COUNT_1_BIT,
#endif
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	
	VkAttachmentReference depthAttachmentRef{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	
	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colourAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef
	};
	
#ifdef MSAA
	VkAttachmentDescription colorAttachmentResolve{
		.format = swapChainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};
	VkAttachmentReference colorAttachmentResolveRef{
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
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
	
	VkSubpassDependency dependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	if(vkCreateRenderPass(blueprint.devices.logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) throw std::runtime_error("failed to create render pass!");
	
	
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
	vertexBufferObjects = std::vector<std::optional<VertexBufferObject>>(blueprint.vertexBuffersN, std::optional<VertexBufferObject>());
	
	
	// -----
	// Allocating the index buffer arrays
	// -----
	indexBufferObjects = std::vector<std::optional<IndexBufferObject>>(blueprint.indexBuffersN, std::optional<IndexBufferObject>());
	
	
	// -----
	// Creating frame buffers
	// -----
	CreateFramebuffers();
	
	
	// -----
	// Allocating arrays and building structures
	// -----
	uniformBufferObjects = std::vector<std::optional<UniformBufferObject>>(blueprint.uniformBufferObjectsN, std::optional<UniformBufferObject>());
	
	storageBufferObjects = std::vector<std::optional<StorageBufferObject>>(blueprint.storageBufferObjectsN, std::optional<StorageBufferObject>());
	
	textureSamplers = std::vector<VkSampler>(blueprint.samplerBlueprints.size());
	for(int i=0; i<blueprint.samplerBlueprints.size(); ++i) BuildTextureSampler(i, blueprint.samplerBlueprints[i]);
	
	textureImages = std::vector<std::optional<TextureImage>>(blueprint.imagesN, std::optional<TextureImage>());
	
	resisingBRPs = std::vector<int>();
	resisingImages = std::vector<int>();
	
	bufferedRenderPasses = std::vector<std::optional<BufferedRenderPass>>(blueprint.bufferedRenderPassesN, std::optional<BufferedRenderPass>());
	
	layeredBufferedRenderPasses = std::vector<std::optional<LayeredBufferedRenderPass>>(blueprint.layeredBufferedRenderPassesN, std::optional<LayeredBufferedRenderPass>());
	
	graphicsPipelines = std::vector<std::shared_ptr<GraphicsPipeline>>(blueprint.graphicsPipelinesN, std::shared_ptr<GraphicsPipeline>());
	
	computePipelines = std::vector<std::shared_ptr<ComputePipeline>>(blueprint.computePipelinesN, std::shared_ptr<ComputePipeline>());

	
	// -----
	// Creating sync objects
	// -----
	{
		VkSemaphoreCreateInfo semaphoreInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT // starts the fence off as signalled so it doesn't wait indefinately upon first wait
		};
		
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
			if (vkCreateSemaphore(blueprint.devices.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphoresFlying[i]) != VK_SUCCESS ||
				vkCreateSemaphore(blueprint.devices.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphoresFlying[i]) != VK_SUCCESS ||
				vkCreateFence(blueprint.devices.logicalDevice, &fenceInfo, nullptr, &inFlightFencesFlying[i]) != VK_SUCCESS ||
				vkCreateSemaphore(blueprint.devices.logicalDevice, &semaphoreInfo, nullptr, &computeFinishedSemaphoresFlying[i]) != VK_SUCCESS ||
				vkCreateFence(blueprint.devices.logicalDevice, &fenceInfo, nullptr, &computeInFlightFencesFlying[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronisation objects!");
			}
		}
	}
}
void Interface::RecreateResisingBRPsAndImages(){
	if(resisingImages.empty() && resisingBRPs.empty()) return;
	
	for(int resisingBRP : resisingBRPs){
		for(int j=0; j<MAX_FRAMES_IN_FLIGHT; j++){
			if(bufferedRenderPasses[resisingBRP])
				vkDestroyFramebuffer(devices.logicalDevice, bufferedRenderPasses[resisingBRP]->frameBuffersFlying[j], nullptr);
		}
	}
	for(int resisingImage : resisingImages){
		std::optional<TextureImage> &ref = textureImages[resisingImage];
		if(ref){
			vkDestroyImageView(devices.logicalDevice, ref->view, nullptr);
			vmaDestroyImage(devices.allocator, ref->image, ref->allocation);
		}
	}
	
	for(int resisingImage : resisingImages){
		std::optional<TextureImage> &ref = textureImages[resisingImage];
		if(!ref) continue;
		
		ref->blueprint.imageCI.extent.width = swapChainExtent.width;
		ref->blueprint.imageCI.extent.height = swapChainExtent.height;

		CreateImage(ref->blueprint.imageCI, ref->blueprint.properties, ref->image, ref->allocation);
		
		ref->view = CreateImageView({
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = ref->image,
			.viewType = ref->blueprint.imageViewType,
			.format = ref->blueprint.imageCI.format,
			.components = {},
			.subresourceRange = {ref->blueprint.aspectFlags, 0, ref->blueprint.imageCI.mipLevels, 0, ref->blueprint.imageCI.arrayLayers}
		});
	}
	for(int resisingBRP : resisingBRPs){
		std::optional<BufferedRenderPass> &ref = bufferedRenderPasses[resisingBRP];
		if(!ref) continue;
		
		ref->width = swapChainExtent.width;
		ref->height = swapChainExtent.height;
		
		VkFramebufferCreateInfo frameBufferCI = {};
		frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCI.renderPass = ref->renderPass;
		frameBufferCI.attachmentCount = uint32_t(bufferedRenderPasses[resisingBRP]->targetTextureImageIndices.size());
		VkImageView attachments[bufferedRenderPasses[resisingBRP]->targetTextureImageIndices.size()];
		for(int j=0; j<bufferedRenderPasses[resisingBRP]->targetTextureImageIndices.size(); j++){
			if(!textureImages[bufferedRenderPasses[resisingBRP]->targetTextureImageIndices[j]])
				throw std::runtime_error("Image index to resize doesn't exist");
			attachments[j] = textureImages[bufferedRenderPasses[resisingBRP]->targetTextureImageIndices[j]]->view;
		}
		frameBufferCI.pAttachments = attachments;
		frameBufferCI.width = ref->width;
		frameBufferCI.height = ref->height;
		frameBufferCI.layers = 1;
		for(int j=0; j<MAX_FRAMES_IN_FLIGHT; j++){
			if(vkCreateFramebuffer(devices.logicalDevice, &frameBufferCI, nullptr, &ref->frameBuffersFlying[j]) != VK_SUCCESS)
				throw std::runtime_error("failed to create framebuffer!");
		}
	}
	
	for(std::shared_ptr<GraphicsPipeline> graphicsPipeline : graphicsPipelines) if(graphicsPipeline) graphicsPipeline->UpdateDescriptorSets();
}
void Interface::CleanUpSwapChain(){
	vkDestroyImageView(devices.logicalDevice, depthImageView, nullptr);
	vmaDestroyImage(devices.allocator, depthImage, depthImageAllocation);
	
#ifdef MSAA
	vkDestroyImageView(devices.logicalDevice, colourImageView, nullptr);
	vmaDestroyImage(devices.allocator, colourImage, colourImageAllocation);
#endif
	
	vkDestroySwapchainKHR(devices.logicalDevice, swapChain, nullptr);
	for(size_t i=0; i<imageCount; i++){
		vkDestroyFramebuffer(devices.logicalDevice, swapChainFramebuffers[i], nullptr);
		vkDestroyImageView(devices.logicalDevice, swapChainImageViews[i], nullptr);
	}
}
Interface::~Interface(){
	vkDeviceWaitIdle(devices.logicalDevice);
	
	CleanUpSwapChain();
	
	for(std::optional<BufferedRenderPass> &brp : bufferedRenderPasses) if(brp) brp->CleanUp(devices.logicalDevice);
	for(std::optional<LayeredBufferedRenderPass> &lbrp : layeredBufferedRenderPasses) if(lbrp) lbrp->CleanUp(devices.logicalDevice);
	for(VkSampler &sampler : textureSamplers) vkDestroySampler(devices.logicalDevice, sampler, nullptr);
	for(std::optional<TextureImage> &textureImage : textureImages) if(textureImage) textureImage->CleanUp(devices.logicalDevice, devices.allocator);
	for(std::optional<UniformBufferObject> &ubo : uniformBufferObjects) if(ubo) ubo->CleanUp(devices.allocator);
	for(std::optional<StorageBufferObject> &sbo : storageBufferObjects) if(sbo) sbo->CleanUp(devices.allocator);
	for(std::optional<VertexBufferObject> &vbo : vertexBufferObjects) if(vbo){ vbo->CleanUp(devices.allocator); }
	for(std::optional<IndexBufferObject> &ibo : indexBufferObjects) if(ibo){ ibo->CleanUp(devices.allocator); }
	for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
		vkDestroySemaphore(devices.logicalDevice, imageAvailableSemaphoresFlying[i], nullptr);
		vkDestroySemaphore(devices.logicalDevice, renderFinishedSemaphoresFlying[i], nullptr);
		vkDestroyFence(devices.logicalDevice, inFlightFencesFlying[i], nullptr);
		vkDestroySemaphore(devices.logicalDevice, computeFinishedSemaphoresFlying[i], nullptr);
		vkDestroyFence(devices.logicalDevice, computeInFlightFencesFlying[i], nullptr);
	}
	
	vkDestroyRenderPass(devices.logicalDevice, renderPass, nullptr);
	vkDestroyCommandPool(devices.logicalDevice, commandPool, nullptr);
}
void Interface::CreateAndFillDeviceLocalBuffer(VkBuffer &bufferHandle, VmaAllocation &allocation, void *data, const VkDeviceSize &size, const VkBufferUsageFlags &usageFlags){
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
void Interface::FillExistingDeviceLocalBuffer(VkBuffer bufferHandle, void *data, const VkDeviceSize &size){
	// creating staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	
	memcpy(stagingAllocInfo.pMappedData, data, (size_t)size);
	vmaFlushAllocation(devices.allocator, stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// copying the contents of the staging buffer into the vertex buffer
	CopyBuffer(stagingBuffer, bufferHandle, size);
	// cleaning up staging buffer
	vmaDestroyBuffer(devices.allocator, stagingBuffer, stagingAllocation);
}
void Interface::FillVertexBuffer(int vertexBufferIndex, void *vertices, const VkDeviceSize &size, const VkDeviceSize &offset){
	// destroying old vertex buffer if it exists
	if(vertexBufferObjects[vertexBufferIndex]){
		if(vertexBufferObjects[vertexBufferIndex]->size == size){
			FillExistingDeviceLocalBuffer(vertexBufferObjects[vertexBufferIndex]->bufferHandle, vertices, size);
			vertexBufferObjects[vertexBufferIndex]->offset = offset;
			return;
		} else {
			vmaDestroyBuffer(devices.allocator, vertexBufferObjects[vertexBufferIndex]->bufferHandle, vertexBufferObjects[vertexBufferIndex]->allocation);
		}
	} else {
		vertexBufferObjects[vertexBufferIndex] = VertexBufferObject();
	}
	
	CreateAndFillDeviceLocalBuffer(vertexBufferObjects[vertexBufferIndex]->bufferHandle, vertexBufferObjects[vertexBufferIndex]->allocation, vertices, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	
	vertexBufferObjects[vertexBufferIndex]->offset = offset;
	vertexBufferObjects[vertexBufferIndex]->size = size;
}
void Interface::FillIndexBuffer(int indexBufferIndex, uint32_t *indices, size_t indexCount, const VkDeviceSize &offset){
	// destroying old index buffer if it exists
	if(indexBufferObjects[indexBufferIndex]){
		if(indexBufferObjects[indexBufferIndex]->indexCount == indexCount){
			FillExistingDeviceLocalBuffer(indexBufferObjects[indexBufferIndex]->bufferHandle, indices, VkDeviceSize(indexCount * sizeof(int32_t)));
			indexBufferObjects[indexBufferIndex]->offset = offset;
			return;
		} else {
			vmaDestroyBuffer(devices.allocator, indexBufferObjects[indexBufferIndex]->bufferHandle, indexBufferObjects[indexBufferIndex]->allocation);
		}
	} else {
		indexBufferObjects[indexBufferIndex] = IndexBufferObject();
	}
	
	CreateAndFillDeviceLocalBuffer(indexBufferObjects[indexBufferIndex]->bufferHandle, indexBufferObjects[indexBufferIndex]->allocation, indices, VkDeviceSize(indexCount * sizeof(uint32_t)), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	
	indexBufferObjects[indexBufferIndex]->offset = offset;
	indexBufferObjects[indexBufferIndex]->indexCount = (uint32_t)indexCount;
}
void Interface::FillStorageBuffer(int storageBufferIndex, void *data){
	std::optional<StorageBufferObject> &ref = storageBufferObjects[storageBufferIndex];
	if(!ref)
		throw std::runtime_error("Cannot fill storage buffer; it hasn't been created yet");
	
	for(int i=0; i<MAX_FRAMES_IN_FLIGHT; ++i){
		FillExistingDeviceLocalBuffer(ref->buffersFlying[i], data, ref->size);
	}
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
		return false;
	} else if(result != VK_SUCCESS) throw std::runtime_error("failed to acquire swap chain image!");
	
	// only reset the fence if we are submitting work
	vkResetFences(devices.logicalDevice, 1, &inFlightFencesFlying[currentFrame]);
	
	// recording the command buffer
	vkResetCommandBuffer(commandBuffersFlying[currentFrame], 0);
	
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // Optional; this bit means we will submit this buffer once between each reset
		.pInheritanceInfo = nullptr // Optional
	};
	if(vkBeginCommandBuffer(commandBuffersFlying[currentFrame], &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording command buffer!");
	
	return true;
}
void Interface::BeginFinalRenderPass(const VkClearColorValue &clearColour){
	VkClearValue clearValues[2] = {};
	clearValues[0].color = clearColour;
	clearValues[1].depthStencil = {1.0f, 0};
	
	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = swapChainFramebuffers[currentFrameImageIndex],
		.renderArea = (VkRect2D){
			.offset = {0, 0},
			.extent = swapChainExtent
		},
		.clearValueCount = 2,
		.pClearValues = clearValues
	};
	vkCmdBeginRenderPass(commandBuffersFlying[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(swapChainExtent.width),
		.height = float(swapChainExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
	
	// can filter at rasterizer stage to change rendered rectangle within viewport
	VkRect2D scissor{
		.offset = {0, 0},
		.extent = swapChainExtent
	};
	vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
}
void Interface::EndFinalRenderPassAndFrame(std::optional<VkPipelineStageFlags> stagesWaitForCompute){
	vkCmdEndRenderPass(commandBuffersFlying[currentFrame]);
	
	if(vkEndCommandBuffer(commandBuffersFlying[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to record command buffer!");
	
	// submitting the command buffer to the graphics queue
	std::vector<VkSemaphore> waitSemaphores = {imageAvailableSemaphoresFlying[currentFrame]};
	std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	if(stagesWaitForCompute){
		waitSemaphores.push_back(computeFinishedSemaphoresFlying[currentFrame]);
		waitStages.push_back(stagesWaitForCompute.value());
	}
	VkSemaphore signalSemaphores[1] = {renderFinishedSemaphoresFlying[currentFrame]};
	
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = uint32_t(waitSemaphores.size()),
		.pWaitSemaphores = waitSemaphores.data(),
		.pWaitDstStageMask = waitStages.data(),
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffersFlying[currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores
	};
	if(vkQueueSubmit(devices.graphicsQueue, 1, &submitInfo, inFlightFencesFlying[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer!");
	
	// presenting on the present queue
	VkSwapchainKHR swapChains[] = {swapChain};
	
	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &currentFrameImageIndex,
		.pResults = nullptr // Optional
	};
	const VkResult result = vkQueuePresentKHR(devices.presentQueue, &presentInfo);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		RecreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
void Interface::BeginCompute(){
	vkWaitForFences(devices.logicalDevice, 1, &computeInFlightFencesFlying[currentFrame], VK_TRUE, UINT64_MAX);

//	updateUniformBuffer(currentFrame);

	vkResetFences(devices.logicalDevice, 1, &computeInFlightFencesFlying[currentFrame]);

	vkResetCommandBuffer(computeCommandBuffersFlying[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
	
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};
	if (vkBeginCommandBuffer(computeCommandBuffersFlying[currentFrame], &beginInfo) != VK_SUCCESS)
		throw std::runtime_error("failed to begin recording compute command buffer!");
}
void Interface::EndCompute(){
	if(vkEndCommandBuffer(computeCommandBuffersFlying[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to record command buffer!");
	
	VkSubmitInfo submitInfo {
		.commandBufferCount = 1,
		.pCommandBuffers = &computeCommandBuffersFlying[currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &computeFinishedSemaphoresFlying[currentFrame]
	};
	if (vkQueueSubmit(devices.computeQueue, 1, &submitInfo, computeInFlightFencesFlying[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to submit compute command buffer!");
}

void Interface::CmdEndRenderPass(){
	vkCmdEndRenderPass(commandBuffersFlying[currentFrame]);
}

void Interface::CmdBindVertexBuffer(uint32_t binding, int index){
	if(!vertexBufferObjects[index]) throw std::runtime_error("Cannot bind vertex buffer; it hasn't been filled.");
	vkCmdBindVertexBuffers(commandBuffersFlying[currentFrame], binding, 1, &vertexBufferObjects[index]->bufferHandle, &vertexBufferObjects[index]->offset);
}
void Interface::CmdBindStorageBufferAsVertexBuffer(uint32_t binding, int index, const VkDeviceSize &offset){
	vkCmdBindVertexBuffers(commandBuffersFlying[currentFrame], binding, 1, &storageBufferObjects[index]->buffersFlying[currentFrame], &offset);
}
void Interface::CmdBindIndexBuffer(int index, const VkIndexType &type){
	if(!indexBufferObjects[index]) throw std::runtime_error("Cannot bind index buffer; it hasn't been filled.");
	vkCmdBindIndexBuffer(commandBuffersFlying[currentFrame], indexBufferObjects[index]->bufferHandle, indexBufferObjects[index]->offset, type);
}
void Interface::CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance){
	vkCmdDraw(commandBuffersFlying[currentFrame], vertexCount, instanceCount, firstVertex, firstInstance);
}
void Interface::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance){
	vkCmdDrawIndexed(commandBuffersFlying[currentFrame], indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
void Interface::CmdSetDepthBias(float constantFactor, float clamp, float slopeFactor){
	vkCmdSetDepthBias(commandBuffersFlying[currentFrame], constantFactor, clamp, slopeFactor);
}
void Interface::CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ){
	vkCmdDispatch(computeCommandBuffersFlying[currentFrame], groupCountX, groupCountY, groupCountZ);
}
void Interface::CmdPipelineImageMemoryBarrier(bool graphicsOrCompute, int imageIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkImageSubresourceRange subresourceRange){
	if(!textureImages[imageIndex])
		throw std::runtime_error("Cannot create image memory barrier as image has not been created.");
	TextureImage &imageRef = textureImages[imageIndex].value();
	
	const VkImageMemoryBarrier imageMemoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.image = imageRef.image,
		.subresourceRange = subresourceRange,
		.srcAccessMask = srcAccessMask,
		.dstAccessMask = dstAccessMask,
		.srcQueueFamilyIndex = srcQueueFamilyIndex,
		.dstQueueFamilyIndex = dstQueueFamilyIndex
	};
	vkCmdPipelineBarrier(graphicsOrCompute ? computeCommandBuffersFlying[currentFrame] : commandBuffersFlying[currentFrame],
						 srcStageMask, dstStageMask, dependencyFlags,
						 0, nullptr, 0, nullptr,
						 1, &imageMemoryBarrier);
}
void Interface::CmdBeginBufferedRenderPass(int bufferedRenderPassIndex, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues){
	if(!bufferedRenderPasses[bufferedRenderPassIndex])
		throw std::runtime_error("Cannot begin buffered render pass; it hasn't been created.");
	BufferedRenderPass &ref = bufferedRenderPasses[bufferedRenderPassIndex].value();
	
	VkRenderPassBeginInfo renderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = ref.renderPass,
		.framebuffer = ref.frameBuffersFlying[currentFrame],
		.renderArea = {{0, 0}, {ref.width, ref.height}},
		.clearValueCount = uint32_t(clearValues.size()),
		.pClearValues = clearValues.data()
	};
	vkCmdBeginRenderPass(commandBuffersFlying[currentFrame], &renderPassBeginInfo, subpassContents);
	
	VkViewport viewport{
		.x = (float)renderPassBeginInfo.renderArea.offset.x,
		.y = (float)renderPassBeginInfo.renderArea.offset.y,
		.width = (float)renderPassBeginInfo.renderArea.extent.width,
		.height = (float)renderPassBeginInfo.renderArea.extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
	
	// can filter at rasterizer stage to change rendered rectangle within viewport
	VkRect2D scissor{
		.offset = renderPassBeginInfo.renderArea.offset,
		.extent = renderPassBeginInfo.renderArea.extent
	};
	vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
}
void Interface::CmdBeginLayeredBufferedRenderPass(int layeredBufferedRenderPassIndex, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues, int layer){
	if(!layeredBufferedRenderPasses[layeredBufferedRenderPassIndex])
		throw std::runtime_error("Cannot begin layered buffered render pass; it hasn't been created.");
	LayeredBufferedRenderPass &ref = layeredBufferedRenderPasses[layeredBufferedRenderPassIndex].value();
	
	VkRenderPassBeginInfo renderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = ref.renderPass,
		.framebuffer = ref.layers[layer].frameBuffersFlying[currentFrame],
		.renderArea = {{0, 0}, {ref.width, ref.height}},
		.clearValueCount = uint32_t(clearValues.size()),
		.pClearValues = clearValues.data()
	};
	vkCmdBeginRenderPass(commandBuffersFlying[currentFrame], &renderPassBeginInfo, subpassContents);
	
	VkViewport viewport{
		.x = (float)renderPassBeginInfo.renderArea.offset.x,
		.y = (float)renderPassBeginInfo.renderArea.offset.y,
		.width = (float)renderPassBeginInfo.renderArea.extent.width,
		.height = (float)renderPassBeginInfo.renderArea.extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
	
	// can filter at rasterizer stage to change rendered rectangle within viewport
	VkRect2D scissor{
		.offset = renderPassBeginInfo.renderArea.offset,
		.extent = renderPassBeginInfo.renderArea.extent
	};
	vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
}

void Interface::BuildGraphicsPipeline(int index, const GraphicsPipelineBlueprint &blueprint){
	if(graphicsPipelines[index]) graphicsPipelines[index].reset();
	graphicsPipelines[index] = std::make_shared<GraphicsPipeline>(*this, blueprint);
}
void Interface::BuildComputePipeline(int index, const ComputePipelineBlueprint &blueprint){
	if(computePipelines[index]) computePipelines[index].reset();
	computePipelines[index] = std::make_shared<ComputePipeline>(*this, blueprint);
}

void Interface::BuildUBO(int index, const UniformBufferObjectBlueprint &blueprint){
	std::optional<UniformBufferObject> &newUBORef = uniformBufferObjects[index];
	if(newUBORef){
		newUBORef->CleanUp(devices.allocator);
	}
	newUBORef = UniformBufferObject();
	
	bool dynamic = false;
	if(blueprint.dynamicRepeats){
		if(blueprint.dynamicRepeats.value() > 1){
			dynamic = true;
		}
	}
	
	if(dynamic){ // dynamic
		// Calculate required alignment based on minimum device offset alignment
		const VkDeviceSize minUboAlignment = devices.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
		
		newUBORef->dynamic = (UniformBufferObject::Dynamic){
			.repeatsN = blueprint.dynamicRepeats.value(),
			.alignment = minUboAlignment > 0 ? (blueprint.size + minUboAlignment - 1) & ~(minUboAlignment - 1) : blueprint.size
		};
		newUBORef->size = newUBORef->dynamic->alignment * newUBORef->dynamic->repeatsN;
	} else { // not dynamic
		newUBORef->dynamic.reset();
		newUBORef->size = blueprint.size;
	}
	
	for(size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
		// creating buffer
		CreateBuffer(newUBORef->size,
					 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					 newUBORef->buffersFlying[i],
					 newUBORef->allocationsFlying[i],
					 &(newUBORef->allocationInfosFlying[i]));
	}
}
void Interface::BuildSBO(int index, const StorageBufferObjectBlueprint &blueprint){
	std::optional<StorageBufferObject> &newSBORef = storageBufferObjects[index];
	if(newSBORef){
		newSBORef->CleanUp(devices.allocator);
	}
	newSBORef = StorageBufferObject();
	
	newSBORef->size = blueprint.size;
	
	for(size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
		// creating buffer
		CreateBuffer(newSBORef->size,
					 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | blueprint.usages,
					 blueprint.memoryProperties,
					 newSBORef->buffersFlying[i],
					 newSBORef->allocationsFlying[i],
					 &(newSBORef->allocationInfosFlying[i]));
	}
}
void Interface::BuildTextureImage(int index, ManualImageBlueprint blueprint){
	std::optional<TextureImage> &ref = textureImages[index];
	if(ref){
		ref->CleanUp(devices.logicalDevice, devices.allocator);
	}
	ref = TextureImage();
	ref->blueprint = blueprint;
	
	if(blueprint.imageCI.extent.width == 0 || blueprint.imageCI.extent.height == 0){
		blueprint.imageCI.extent.width = swapChainExtent.width;
		blueprint.imageCI.extent.height = swapChainExtent.height;
	}
	
	CreateImage(blueprint.imageCI, blueprint.properties, ref->image, ref->allocation);
	
	ref->view = CreateImageView({
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = ref->image,
		.viewType = blueprint.imageViewType,
		.format = blueprint.imageCI.format,
		.components = {},
		.subresourceRange = {blueprint.aspectFlags, 0, blueprint.imageCI.mipLevels, 0, blueprint.imageCI.arrayLayers}
	});
}
void Interface::BuildTextureImageFromFile(int index, const PNGImageBlueprint &blueprint){
	// loading image onto an sdl devices.surface
	SDL_Surface *const surface = IMG_Load(blueprint.imageFilename.c_str());
	
	if(!surface) throw std::runtime_error("failed to load texture image!");
	
	BuildDataImage(index, {(uint8_t *)surface->pixels, uint32_t(surface->w), uint32_t(surface->h), uint32_t(surface->pitch), VK_FORMAT_R8G8B8A8_SRGB, true});
	//SDLPixelFormatToVulkanFormat((SDL_PixelFormatEnum)devices.surface->format->format); // 24 bit-depth images don't seem to work
	
	SDL_FreeSurface(surface);
}
void Interface::BuildDataImage(int index, const DataImageBlueprint &blueprint){
	std::optional<TextureImage> &ref = textureImages[index];
	if(ref){
		ref->CleanUp(devices.logicalDevice, devices.allocator);
	}
	ref = TextureImage();
	
	const VkDeviceSize imageSize = blueprint.height * blueprint.pitch;
	
	// calculated number of mipmap levels
	ref->mipLevels = blueprint.mip ? (uint32_t)floor(log2((double)(blueprint.width > blueprint.height ? blueprint.width : blueprint.height))) + 1 : 1;
	
	// creating a staging buffer, copying in the pixels and then freeing the SDL devices.surface and the staging buffer memory allocation
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	memcpy(stagingAllocInfo.pMappedData, blueprint.data, (size_t)imageSize);
	vmaFlushAllocation(devices.allocator, stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// creating the image
	VkImageCreateInfo imageCI = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = blueprint.width,
		.extent.height = blueprint.height,
		.extent.depth = 1,
		.mipLevels = ref->mipLevels,
		.arrayLayers = 1,
		.format = blueprint.format,
		.tiling = VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_TILING_LINEAR for row-major order if we want to access texels in the memory of the image
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ref->image, ref->allocation);
	
	VkImageSubresourceRange subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = ref->mipLevels,
		.baseArrayLayer = 0,
		.layerCount = 1
	};
	TransitionImageLayout(ref->image, blueprint.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	CopyBufferToImage(stagingBuffer, ref->image, blueprint.width, blueprint.height);
	GenerateMipmaps(ref->image, blueprint.format, blueprint.width, blueprint.height, ref->mipLevels);
	
	vmaDestroyBuffer(devices.allocator, stagingBuffer, stagingAllocation);
	
	// Creating an image view for the texture image
	ref->view = CreateImageView({
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = ref->image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = blueprint.format,
		.components = {},
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, ref->mipLevels, 0, 1}
	});
	
	ref->blueprint = {imageCI, VK_IMAGE_VIEW_TYPE_2D, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT};
}
void Interface::BuildCubemapImageFromFiles(int index, const CubemapPNGImageBlueprint &blueprint){
	std::optional<TextureImage> &ref = textureImages[index];
	if(ref){
		ref->CleanUp(devices.logicalDevice, devices.allocator);
	}
	ref = TextureImage();
	
	// loading image onto an sdl devices.surface
	std::array<SDL_Surface *, 6> surfaces;
	surfaces[0] = IMG_Load(blueprint.imageFilenames[0].c_str());
	if(!surfaces[0]) throw std::runtime_error("failed to load texture image!");
	const uint32_t faceWidth = surfaces[0]->w;
	//const uint32_t width = 6*faceWidth;
	const uint32_t height = surfaces[0]->h;
	const size_t faceSize = height * surfaces[0]->pitch;
	const VkDeviceSize imageSize = 6 * faceSize;
	const VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;//SDLPixelFormatToVulkanFormat((SDL_PixelFormatEnum)devices.surface->format->format); // 24 bit-depth images don't seem to work
	
	for(int i=1; i<6; i++){
		surfaces[i] = IMG_Load(blueprint.imageFilenames[i].c_str());
		if(!surfaces[i]) throw std::runtime_error("failed to load texture image!");
		assert(surfaces[i]->w == faceWidth);
		assert(surfaces[i]->h == height);
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
	VkImageCreateInfo imageCI = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = faceWidth,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 6,
		.format = imageFormat,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	};
	CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ref->image, ref->allocation);
	
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
	
	VkImageSubresourceRange subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 6
	};
	TransitionImageLayout(ref->image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	
	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, ref->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, regions);
	
	TransitionImageLayout(ref->image, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
	
	EndSingleTimeCommands(commandBuffer);
	
	vmaDestroyBuffer(devices.allocator, stagingBuffer, stagingAllocation);
	
	// Creating an image view for the texture image
	ref->view = CreateImageView({
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = ref->image,
		.viewType = VK_IMAGE_VIEW_TYPE_CUBE,
		.format = imageFormat,
		.components = {},
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6}
	});
	
	// no blueprint as a cube map can't resize with the window
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
