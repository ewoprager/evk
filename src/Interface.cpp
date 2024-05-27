#include <set>
#include <stdio.h>

#define VMA_IMPLEMENTATION // only put this define in one cpp file. put the below include in all cpp files that need it
#include <vma/vk_mem_alloc.h>

#include <Interface.hpp>

namespace EVK {

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


Interface::Interface(std::shared_ptr<Devices> _devices) : devices(_devices) {
	
	// Initialising SDL_image
	IMG_Init(IMG_INIT_PNG);
	
	// -----
	// Allocating command buffers
	// -----
	{
		const VkCommandBufferAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = devices->GetCommandPool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = MAX_FRAMES_IN_FLIGHT
		};
		if(vkAllocateCommandBuffers(devices->GetLogicalDevice(), &allocInfo, commandBuffersFlying) != VK_SUCCESS){
			throw std::runtime_error("failed to allocate command buffers!");
		}
		if(vkAllocateCommandBuffers(devices->GetLogicalDevice(), &allocInfo, computeCommandBuffersFlying) != VK_SUCCESS){
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	
	// -----
	// Creating the swap chain, getting swap chain images and saving chosen format and extent of swap chain
	// -----
	CreateSwapChain(devices->GetSurfaceExtent());
	
	
	// -----
	// Creating swap chain image views
	// -----
	CreateImageViews();
	
	
	// -----
	// Creating the render pass
	// -----
	const VkAttachmentDescription colourAttachment{
		.format = swapChainImageFormat,
#ifdef MSAA
		.samples = devices->msaaSamples,
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
	const VkAttachmentReference colourAttachmentRef{
		.attachment = 0,
#ifdef MSAA
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
#else
		.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
#endif
	};
	const VkAttachmentDescription depthAttachment{
		.format = devices->FindDepthFormat(),
#ifdef MSAA
		.samples = devices->msaaSamples,
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
	const VkAttachmentReference depthAttachmentRef{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	const VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colourAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef
	};
	
#ifdef MSAA
	const VkAttachmentDescription colorAttachmentResolve{
		.format = swapChainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};
	const VkAttachmentReference colorAttachmentResolveRef{
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	subpass.pResolveAttachments = &colorAttachmentResolveRef;
#endif
	
	const VkSubpassDependency dependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};
	
#ifdef MSAA
	const VkAttachmentDescription attachments[3] = {colourAttachment, depthAttachment, colorAttachmentResolve};
#else
	const VkAttachmentDescription attachments[2] = {colourAttachment, depthAttachment};
#endif
	const VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
#ifdef MSAA
		.attachmentCount = 3,
#else
		.attachmentCount = 2,
#endif
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};
	if(vkCreateRenderPass(devices->GetLogicalDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS){
		throw std::runtime_error("failed to create render pass!");
	}
	
	
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
	// Creating frame buffers
	// -----
	CreateFramebuffers();

	
	// -----
	// Creating sync objects
	// -----
	{
		const VkSemaphoreCreateInfo semaphoreInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		const VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT // starts the fence off as signalled so it doesn't wait indefinately upon first wait
		};
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; ++i){
			if (vkCreateSemaphore(devices->GetLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphoresFlying[i]) != VK_SUCCESS ||
				vkCreateSemaphore(devices->GetLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphoresFlying[i]) != VK_SUCCESS ||
				vkCreateFence(devices->GetLogicalDevice(), &fenceInfo, nullptr, &inFlightFencesFlying[i]) != VK_SUCCESS ||
				vkCreateSemaphore(devices->GetLogicalDevice(), &semaphoreInfo, nullptr, &computeFinishedSemaphoresFlying[i]) != VK_SUCCESS ||
				vkCreateFence(devices->GetLogicalDevice(), &fenceInfo, nullptr, &computeInFlightFencesFlying[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create synchronisation objects!");
			}
		}
	}
}

Interface::~Interface(){
	vkDeviceWaitIdle(devices->GetLogicalDevice());
	
	CleanUpSwapChain();
	
	for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
		vkDestroySemaphore(devices->GetLogicalDevice(), imageAvailableSemaphoresFlying[i], nullptr);
		vkDestroySemaphore(devices->GetLogicalDevice(), renderFinishedSemaphoresFlying[i], nullptr);
		vkDestroyFence(devices->GetLogicalDevice(), inFlightFencesFlying[i], nullptr);
		vkDestroySemaphore(devices->GetLogicalDevice(), computeFinishedSemaphoresFlying[i], nullptr);
		vkDestroyFence(devices->GetLogicalDevice(), computeInFlightFencesFlying[i], nullptr);
	}
	vkDestroyRenderPass(devices->GetLogicalDevice(), renderPass, nullptr);
}

void Interface::CreateSwapChain(const VkExtent2D &actualExtent){
	SwapChainSupportDetails swapChainSupport = devices->QuerySwapChainSupport();
	
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
		createInfo.surface = devices->Surface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		
		uint32_t queueFamilyIndicesArray[] = {
			devices->GetQueueFamilyIndices().graphicsAndComputeFamily.value(),
			devices->GetQueueFamilyIndices().presentFamily.value()
		};
		
		if(devices->GetQueueFamilyIndices().graphicsAndComputeFamily != devices->GetQueueFamilyIndices().presentFamily){
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
		
		if(vkCreateSwapchainKHR(devices->GetLogicalDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS){
			throw std::runtime_error("failed to create swap chain!");
		}
	}
	
	// Getting swap chain images
	vkGetSwapchainImagesKHR(devices->GetLogicalDevice(), swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(devices->GetLogicalDevice(), swapChain, &imageCount, swapChainImages.data());
	
	// Saving chosen format and extent of swap chain
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Interface::CreateImageViews(){
	swapChainImageViews.resize(imageCount);
	for(size_t i=0; i<imageCount; i++){
		swapChainImageViews[i] = devices->CreateImageView({
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
		.samples = devices->msaaSamples,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	devices->CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colourImage, colourImageAllocation);
	
	colourImageView = devices->CreateImageView({
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
	VkFormat depthFormat = devices->FindDepthFormat();
	
	const VkImageCreateInfo imageCI = {
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
		.samples = devices->msaaSamples,
#else
		.samples = VK_SAMPLE_COUNT_1_BIT,
#endif
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	devices->CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageAllocation);
	
	depthImageView = devices->CreateImageView({
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
	
	for(size_t i=0; i<imageCount; ++i){
#ifdef MSAA
		VkImageView attachments[3] = {
			colourImageView,
			depthImageView,
			swapChainImageViews[i]
		};
#else
		VkImageView attachments[2] = {
			swapChainImageViews[i],
			depthImageView
		};
#endif
		const VkFramebufferCreateInfo framebufferInfo {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
#ifdef MSAA
			.attachmentCount = 2,
#else
			.attachmentCount = 2,
#endif
			.pAttachments = attachments,
			.width = swapChainExtent.width,
			.height = swapChainExtent.height,
			.layers = 1
		};
		if(vkCreateFramebuffer(devices->GetLogicalDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS){
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}
//
//void Interface::ResizeSwapChainSizeMatchingBRPs(){
//	if(swapChainSizeMatchedBRPs.empty()) return;
//	
//	const vec<2, uint32_t> swapChainSize = (vec<2, uint32_t>){swapChainExtent.width, swapChainExtent.height};
//	
//	for(int brpIndex : swapChainSizeMatchedBRPs){
//		AllocateOrResizeBufferedRenderPass(brpIndex, swapChainSize);
//	}
//	
//	for(std::shared_ptr<GraphicsPipeline> graphicsPipeline : graphicsPipelines){
//		if(graphicsPipeline) graphicsPipeline->UpdateDescriptorSets();
//	}
//}

void Interface::CleanUpSwapChain(){
	vkDestroyImageView(devices->GetLogicalDevice(), depthImageView, nullptr);
	vmaDestroyImage(devices->GetAllocator(), depthImage, depthImageAllocation);
	
#ifdef MSAA
	vkDestroyImageView(devices->GetLogicalDevice(), colourImageView, nullptr);
	vmaDestroyImage(devices->allocator, colourImage, colourImageAllocation);
#endif
	
	vkDestroySwapchainKHR(devices->GetLogicalDevice(), swapChain, nullptr);
	for(size_t i=0; i<imageCount; ++i){
		vkDestroyFramebuffer(devices->GetLogicalDevice(), swapChainFramebuffers[i], nullptr);
		vkDestroyImageView(devices->GetLogicalDevice(), swapChainImageViews[i], nullptr);
	}
}

std::optional<Interface::FrameInfo> Interface::BeginFrame(){
	// waiting until previous frame has finished rendering
	vkWaitForFences(devices->GetLogicalDevice(), 1, &inFlightFencesFlying[currentFrame], VK_TRUE, UINT64_MAX);
	
	// acquiring an image from the swap chain
	VkResult result = vkAcquireNextImageKHR(devices->GetLogicalDevice(), swapChain, UINT64_MAX, imageAvailableSemaphoresFlying[currentFrame], VK_NULL_HANDLE, &currentFrameImageIndex);
	
	// checking if we need to recreate the swap chain (e.g. if the window is resized)
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		RecreateSwapChain();
		return {};
	} else if(result != VK_SUCCESS){
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	
	// only reset the fence if we are submitting work
	vkResetFences(devices->GetLogicalDevice(), 1, &inFlightFencesFlying[currentFrame]);
	
	// recording the command buffer
	vkResetCommandBuffer(commandBuffersFlying[currentFrame], 0);
	
	const VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // Optional; this bit means we will submit this buffer once between each reset
		.pInheritanceInfo = nullptr // Optional
	};
	if(vkBeginCommandBuffer(commandBuffersFlying[currentFrame], &beginInfo) != VK_SUCCESS){
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	
	return FrameInfo{commandBuffersFlying[currentFrame], currentFrame};
}
void Interface::BeginFinalRenderPass(const VkClearColorValue &clearColour){
	VkClearValue clearValues[2] = {};
	clearValues[0].color = clearColour;
	clearValues[1].depthStencil = {1.0f, 0};
	
	const VkRenderPassBeginInfo renderPassInfo{
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
	
	const VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = float(swapChainExtent.width),
		.height = float(swapChainExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
	
	// can filter at rasterizer stage to change rendered rectangle within viewport
	const VkRect2D scissor{
		.offset = {0, 0},
		.extent = swapChainExtent
	};
	vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
}
void Interface::EndFinalRenderPassAndFrame(std::optional<VkPipelineStageFlags> stagesWaitForCompute){
	vkCmdEndRenderPass(commandBuffersFlying[currentFrame]);
	
	if(vkEndCommandBuffer(commandBuffersFlying[currentFrame]) != VK_SUCCESS){
		throw std::runtime_error("failed to record command buffer!");
	}
	
	// submitting the command buffer to the graphics queue
	std::vector<VkSemaphore> waitSemaphores = {imageAvailableSemaphoresFlying[currentFrame]};
	std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	if(stagesWaitForCompute){
		waitSemaphores.push_back(computeFinishedSemaphoresFlying[currentFrame]);
		waitStages.push_back(stagesWaitForCompute.value());
	}
	VkSemaphore signalSemaphores[1] = {renderFinishedSemaphoresFlying[currentFrame]};
	
	const VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = uint32_t(waitSemaphores.size()),
		.pWaitSemaphores = waitSemaphores.data(),
		.pWaitDstStageMask = waitStages.data(),
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffersFlying[currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores
	};
	if(vkQueueSubmit(devices->GraphicsQueue(), 1, &submitInfo, inFlightFencesFlying[currentFrame]) != VK_SUCCESS){
		throw std::runtime_error("failed to submit draw command buffer!");
	}
	
	// presenting on the present queue
	VkSwapchainKHR swapChains[] = {swapChain};
	
	const VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &currentFrameImageIndex,
		.pResults = nullptr // Optional
	};
	const VkResult result = vkQueuePresentKHR(devices->PresentQueue(), &presentInfo);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		RecreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
void Interface::BeginCompute(){
	vkWaitForFences(devices->GetLogicalDevice(), 1, &computeInFlightFencesFlying[currentFrame], VK_TRUE, UINT64_MAX);

//	updateUniformBuffer(currentFrame);

	vkResetFences(devices->GetLogicalDevice(), 1, &computeInFlightFencesFlying[currentFrame]);

	vkResetCommandBuffer(computeCommandBuffersFlying[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
	
	const VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};
	if (vkBeginCommandBuffer(computeCommandBuffersFlying[currentFrame], &beginInfo) != VK_SUCCESS){
		throw std::runtime_error("failed to begin recording compute command buffer!");
	}
}
void Interface::EndCompute(){
	if(vkEndCommandBuffer(computeCommandBuffersFlying[currentFrame]) != VK_SUCCESS){
		throw std::runtime_error("failed to record command buffer!");
	}
	const VkSubmitInfo submitInfo {
		.commandBufferCount = 1,
		.pCommandBuffers = &computeCommandBuffersFlying[currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &computeFinishedSemaphoresFlying[currentFrame]
	};
	if (vkQueueSubmit(devices->ComputeQueue(), 1, &submitInfo, computeInFlightFencesFlying[currentFrame]) != VK_SUCCESS){
		throw std::runtime_error("failed to submit compute command buffer!");
	}
}

void Interface::CmdEndRenderPass(){
	vkCmdEndRenderPass(commandBuffersFlying[currentFrame]);
}

//void Interface::CmdBindVertexBuffer(uint32_t binding, int index){
//	if(!vertexBufferObjects[index]) throw std::runtime_error("Cannot bind vertex buffer; it hasn't been filled.");
//	vkCmdBindVertexBuffers(commandBuffersFlying[currentFrame], binding, 1, &vertexBufferObjects[index]->bufferHandle, &vertexBufferObjects[index]->offset);
//}
//void Interface::CmdBindStorageBufferAsVertexBuffer(uint32_t binding, int index, const VkDeviceSize &offset){
//	vkCmdBindVertexBuffers(commandBuffersFlying[currentFrame], binding, 1, &storageBufferObjects[index]->buffersFlying[currentFrame], &offset);
//}
//void Interface::CmdBindIndexBuffer(int index, const VkIndexType &type){
//	if(!indexBufferObjects[index]) throw std::runtime_error("Cannot bind index buffer; it hasn't been filled.");
//	vkCmdBindIndexBuffer(commandBuffersFlying[currentFrame], indexBufferObjects[index]->bufferHandle, indexBufferObjects[index]->offset, type);
//}
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
//void Interface::CmdPipelineImageMemoryBarrier(bool graphicsOrCompute, int imageIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkImageSubresourceRange subresourceRange){
//	if(!textureImages[imageIndex]){
//		throw std::runtime_error("Cannot create image memory barrier as image has not been created.");
//	}
//	TextureImage &imageRef = textureImages[imageIndex].value();
//	
//	const VkImageMemoryBarrier imageMemoryBarrier = {
//		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//		.oldLayout = oldLayout,
//		.newLayout = newLayout,
//		.image = imageRef.image,
//		.subresourceRange = subresourceRange,
//		.srcAccessMask = srcAccessMask,
//		.dstAccessMask = dstAccessMask,
//		.srcQueueFamilyIndex = srcQueueFamilyIndex,
//		.dstQueueFamilyIndex = dstQueueFamilyIndex
//	};
//	vkCmdPipelineBarrier(graphicsOrCompute ? computeCommandBuffersFlying[currentFrame] : commandBuffersFlying[currentFrame],
//						 srcStageMask, dstStageMask, dependencyFlags,
//						 0, nullptr, 0, nullptr,
//						 1, &imageMemoryBarrier);
//}
//void Interface::CmdBeginBufferedRenderPass(int bufferedRenderPassIndex, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues){
//	if(!bufferedRenderPasses[bufferedRenderPassIndex])
//		throw std::runtime_error("Cannot begin buffered render pass; it hasn't been built.");
//	BufferedRenderPass &ref = bufferedRenderPasses[bufferedRenderPassIndex].value();
//	if(!ref.size)
//		throw std::runtime_error("Cannot begin buffered render pass; it hasn't been allocated.");
//	
//	VkRenderPassBeginInfo renderPassBeginInfo{
//		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
//		.renderPass = ref.renderPass,
//		.framebuffer = ref.frameBuffersFlying[currentFrame],
//		.renderArea = {{0, 0}, {ref.size->x, ref.size->y}},
//		.clearValueCount = uint32_t(clearValues.size()),
//		.pClearValues = clearValues.data()
//	};
//	vkCmdBeginRenderPass(commandBuffersFlying[currentFrame], &renderPassBeginInfo, subpassContents);
//	
//	const VkViewport viewport{
//		.x = (float)renderPassBeginInfo.renderArea.offset.x,
//		.y = (float)renderPassBeginInfo.renderArea.offset.y,
//		.width = (float)renderPassBeginInfo.renderArea.extent.width,
//		.height = (float)renderPassBeginInfo.renderArea.extent.height,
//		.minDepth = 0.0f,
//		.maxDepth = 1.0f
//	};
//	vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
//	
//	// can filter at rasterizer stage to change rendered rectangle within viewport
//	VkRect2D scissor{
//		.offset = renderPassBeginInfo.renderArea.offset,
//		.extent = renderPassBeginInfo.renderArea.extent
//	};
//	vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
//}
//void Interface::CmdBeginLayeredBufferedRenderPass(int layeredBufferedRenderPassIndex, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues, int layer){
//	if(!layeredBufferedRenderPasses[layeredBufferedRenderPassIndex])
//		throw std::runtime_error("Cannot begin layered buffered render pass; it hasn't been created.");
//	LayeredBufferedRenderPass &ref = layeredBufferedRenderPasses[layeredBufferedRenderPassIndex].value();
//	
//	VkRenderPassBeginInfo renderPassBeginInfo{
//		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
//		.renderPass = ref.renderPass,
//		.framebuffer = ref.layers[layer].frameBuffersFlying[currentFrame],
//		.renderArea = {{0, 0}, {ref.width, ref.height}},
//		.clearValueCount = uint32_t(clearValues.size()),
//		.pClearValues = clearValues.data()
//	};
//	vkCmdBeginRenderPass(commandBuffersFlying[currentFrame], &renderPassBeginInfo, subpassContents);
//	
//	const VkViewport viewport{
//		.x = (float)renderPassBeginInfo.renderArea.offset.x,
//		.y = (float)renderPassBeginInfo.renderArea.offset.y,
//		.width = (float)renderPassBeginInfo.renderArea.extent.width,
//		.height = (float)renderPassBeginInfo.renderArea.extent.height,
//		.minDepth = 0.0f,
//		.maxDepth = 1.0f
//	};
//	vkCmdSetViewport(commandBuffersFlying[currentFrame], 0, 1, &viewport);
//	
//	// can filter at rasterizer stage to change rendered rectangle within viewport
//	const VkRect2D scissor{
//		.offset = renderPassBeginInfo.renderArea.offset,
//		.extent = renderPassBeginInfo.renderArea.extent
//	};
//	vkCmdSetScissor(commandBuffersFlying[currentFrame], 0, 1, &scissor);
//}

//void Interface::BuildGraphicsPipeline(int index, const GraphicsPipelineBlueprint &blueprint){
//	if(graphicsPipelines[index]) graphicsPipelines[index].reset();
//	graphicsPipelines[index] = std::make_shared<GraphicsPipeline>(*this, blueprint);
//}
//void Interface::BuildComputePipeline(int index, const ComputePipelineBlueprint &blueprint){
//	if(computePipelines[index]) computePipelines[index].reset();
//	computePipelines[index] = std::make_shared<ComputePipeline>(*this, blueprint);
//}

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
