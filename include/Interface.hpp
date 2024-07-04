#pragma once

#include <iostream>
#include <assert.h>
#include <chrono>
#include <thread>
#include <functional>
#include <set>
#include <optional>

#include "Devices.hpp"

namespace EVK {

class Interface;

//VkFormat SDLPixelFormatToVulkanFormat(const SDL_PixelFormatEnum &sdlPixelFormat);

class Interface {
public:
	Interface(std::shared_ptr<Devices> _devices);
	~Interface();
	
	void SetResizeCallback(const std::function<void(const vec<2, uint32_t> &)> &newValue){
		resizeCallback = newValue;
	}

	// Graphics commands
	// -----
	[[nodiscard]] std::optional<CommandEnvironment> BeginFrame();
//	void CmdEndRenderPass();
	void BeginSwapChainRenderPass(const VkClearColorValue &clearColour={{0.0f, 0.0f, 0.0f, 0.0f}});
	void EndFrame(std::optional<VkPipelineStageFlags> stagesWaitForCompute=std::optional<VkPipelineStageFlags>());
	// ----- Render pass commands -----
//	void CmdDraw(uint32_t vertexCount, uint32_t instanceCount=1, uint32_t firstVertex=0, uint32_t firstInstance=0);
//	void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount=1, uint32_t firstIndex=0, int32_t vertexOffset=0, uint32_t firstInstance=0);
//	void CmdSetDepthBias(float constantFactor, float clamp, float slopeFactor);
	
	// Compute commands
	// -----
	[[nodiscard]] CommandEnvironment BeginCompute();
	void EndCompute();
//	void CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
	
	// Getters
	// -----
	[[nodiscard]] const VkRenderPass &GetRenderPassHandle() const { return renderPass; }
	[[nodiscard]] const uint32_t &GetExtentWidth() const { return swapChainExtent.width; }
	[[nodiscard]] const uint32_t &GetExtentHeight() const { return swapChainExtent.height; }
	
	
	// Misc
	// -----
	void FramebufferResizeCallback(){ framebufferResized = true; }
	
private:
	std::shared_ptr<Devices> devices;
	
	// a command buffer for each flying frame
	VkCommandBuffer commandBuffersFlying[MAX_FRAMES_IN_FLIGHT];
	VkCommandBuffer computeCommandBuffersFlying[MAX_FRAMES_IN_FLIGHT];
	
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	
	uint32_t imageCount;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass;
	bool framebufferResized = false;
	
	// semaphores and fences for each flying frame
	VkSemaphore imageAvailableSemaphoresFlying[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphoresFlying[MAX_FRAMES_IN_FLIGHT];
	VkFence inFlightFencesFlying[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore computeFinishedSemaphoresFlying[MAX_FRAMES_IN_FLIGHT];
	VkFence computeInFlightFencesFlying[MAX_FRAMES_IN_FLIGHT];
	
	// flying frames
	uint32_t currentFrameImageIndex;
	uint32_t currentFrame = 0;
	
	// Depth image
	VkImage depthImage;
	VmaAllocation depthImageAllocation;
	VkImageView depthImageView;
	
#ifdef MSAA
	// Colour image (for MSAA)
	VkImage colourImage;
	VmaAllocation colourImageAllocation;
	VkImageView colourImageView;
#endif
	
	void CreateSwapChain(const VkExtent2D &actualExtent);
	void CreateImageViews();
#ifdef MSAA
	void CreateColourResources();
#endif
	void CreateDepthResources();
	void CreateFramebuffers();
	void CleanUpSwapChain();
	void ResizeSwapChainSizeMatchingBRPs();
	void RecreateSwapChain(){
		VkExtent2D extent = devices->GetSurfaceExtent();
		// in case we are minimised:
		while(extent.width == 0 || extent.height == 0){
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			extent = devices->GetSurfaceExtent();
		}
		
		vkDeviceWaitIdle(devices->GetLogicalDevice());
		
		CleanUpSwapChain();
		
		CreateSwapChain(extent);
		CreateImageViews();
#ifdef MSAA
		CreateColourResources();
#endif
		CreateDepthResources();
		CreateFramebuffers();
		
		if(resizeCallback){
			resizeCallback((vec<2, uint32_t>){swapChainExtent.width, swapChainExtent.height});
		}
	}
	
	std::function<void(const vec<2, uint32_t> &)> resizeCallback {};
};


} // namespace EVK
