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
	
	void SetResizeCallback(std::function<void(const vec<2, uint32_t> &)> newValue){
		resizeCallback = std::move(newValue);
	}
	
	// -----------------
	// ----- Setup -----
	// -----------------
	
	// ----- Filling buffers -----
//	void FillVertexBuffer(int vertexBufferIndex, void *vertices, const VkDeviceSize &size, const VkDeviceSize &offset=0);
//	void FillIndexBuffer(int indexBufferIndex, uint32_t *indices, size_t indexCount, const VkDeviceSize &offset=0);
//	void FillStorageBuffer(int storageBufferIndex, void *data);
	
	
//	// ----- Modifying UBO data -----
//	template <typename T> T *GetUniformBufferObjectPointer(int uniformBufferObjectIndex) const {
//		if(!uniformBufferObjects[uniformBufferObjectIndex])
//			throw std::runtime_error("Cannot get UBO pointer; UBO not created");
//		return (T *)uniformBufferObjects[uniformBufferObjectIndex]->allocationInfosFlying[currentFrame].pMappedData;
//	}
//	template <typename T> std::vector<T *> GetUniformBufferObjectPointers(int uniformBufferObjectIndex) const {
//		if(!uniformBufferObjects[uniformBufferObjectIndex])
//			throw std::runtime_error("Cannot get UBO pointers; UBO not created");
//		std::vector<T *> ret {};
//		if(uniformBufferObjects[uniformBufferObjectIndex]->dynamic){
//			const int number = uniformBufferObjects[uniformBufferObjectIndex]->dynamic->repeatsN;
//			ret.resize(number);
//			uint8_t *start = (uint8_t *)uniformBufferObjects[uniformBufferObjectIndex]->allocationInfosFlying[currentFrame].pMappedData;
//			for(T* &ptr : ret){
//				ptr = (T *)start;
//				start += uniformBufferObjects[uniformBufferObjectIndex]->dynamic->alignment;
//			}
//		} else {
//			ret.push_back((T *)uniformBufferObjects[uniformBufferObjectIndex]->allocationInfosFlying[currentFrame].pMappedData);
//		}
//		return ret;
//	}
	
	
	// ----- Structures -----
//	void BuildUBO(int index, const UniformBufferObjectBlueprint &blueprint);
//	void BuildSBO(int index, const StorageBufferObjectBlueprint &blueprint);
//	void BuildTextureSampler(int index, const VkSamplerCreateInfo &samplerCI);
//	void BuildBufferedRenderPass(int index, const BufferedRenderPassBlueprint &blueprint);
//	void AllocateOrResizeBufferedRenderPass(int index, const vec<2, uint32_t> &size);
//	void BuildLayeredBufferedRenderPass(int index, const LayeredBufferedRenderPassBlueprint &blueprint);
//	void UpdateLayeredBufferedRenderPass(int index);
	
	
	// ---------------------
	// ----- Pipelines -----
//	// ---------------------
//	void BuildGraphicsPipeline(int index, const GraphicsPipelineBlueprint &blueprint);
//	void BuildComputePipeline(int index, const ComputePipelineBlueprint &blueprint);
//	GraphicsPipeline &GP(int index) const { return *graphicsPipelines[index]; }
//	ComputePipeline &CP(int index) const { return *computePipelines[index]; }
	
	
	// ------------------------------------
	// ----- Frames and render passes -----
	// ------------------------------------
	// ----- General commands -----
//	void CmdPipelineImageMemoryBarrier(bool graphicsOrCompute, int imageIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkImageSubresourceRange subresourceRange);
	// ----- Graphics commands -----
	bool BeginFrame();
//	void CmdBeginBufferedRenderPass(int bufferedRenderPassIndex, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues);
//	void CmdBeginLayeredBufferedRenderPass(int layeredBufferedRenderPassIndex, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues, int layer);
	void CmdEndRenderPass();
	void BeginFinalRenderPass(const VkClearColorValue &clearColour={{0.0f, 0.0f, 0.0f, 0.0f}});
	void EndFinalRenderPassAndFrame(std::optional<VkPipelineStageFlags> stagesWaitForCompute=std::optional<VkPipelineStageFlags>());
	// ----- Render pass commands -----
//	void CmdBindVertexBuffer(uint32_t binding, int index);
//	void CmdBindStorageBufferAsVertexBuffer(uint32_t binding, int index, const VkDeviceSize &offset=0);
//	void CmdBindIndexBuffer(int index, const VkIndexType &type);
	void CmdDraw(uint32_t vertexCount, uint32_t instanceCount=1, uint32_t firstVertex=0, uint32_t firstInstance=0);
	void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount=1, uint32_t firstIndex=0, int32_t vertexOffset=0, uint32_t firstInstance=0);
	void CmdSetDepthBias(float constantFactor, float clamp, float slopeFactor);
	
	// ----- Compute commands -----
	void BeginCompute();
	void EndCompute();
	void CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
	
	
	// ------------------
	// ----- Images -----
	// ------------------
//	void BuildTextureImageFromFile(int index, const PNGImageBlueprint &blueprint);
//	void BuildCubemapImageFromFiles(int index, const CubemapPNGImageBlueprint &blueprint);
//	void BuildTextureImage(int index, ManualImageBlueprint blueprint);
//	void BuildDataImage(int index, const DataImageBlueprint &blueprint);
//	void Build3DDataImage(int index, const Data3DImageBlueprint &blueprint);
//	void AllocateOrResizeImage(int index, const vec<3, uint32_t> &size);
	
	
	// ----- Getters -----
	const VkRenderPass &GetRenderPassHandle() const { return renderPass; }
	const uint32_t &GetExtentWidth() const { return swapChainExtent.width; }
	const uint32_t &GetExtentHeight() const { return swapChainExtent.height; }
//	bool GetVertexBufferCreated(int index) const { return (bool)vertexBufferObjects[index]; }
//	bool GetIndexBufferCreated(int index) const { return (bool)indexBufferObjects[index]; }
//	std::optional<VkDeviceSize> GetUniformBufferObjectDynamicAlignment(int index) const { return uniformBufferObjects[index]->dynamic ? uniformBufferObjects[index]->dynamic.value().alignment : std::optional<VkDeviceSize>(); }
//	size_t GetUniformBufferObjectCount(){ return uniformBufferObjects.size(); }
//	uint32_t GetIndexBufferCount(int index) const {
//		if(!indexBufferObjects[index]) return 0;
//		return indexBufferObjects[index]->indexCount;
//	}
//	const VkRenderPass &GetBufferedRenderPassHandle(int index) const { return bufferedRenderPasses[index]->renderPass; }
//	const VkRenderPass &GetLayeredBufferedRenderPassHandle(int index) const { return layeredBufferedRenderPasses[index]->renderPass; }
//	const VkDevice &GetLogicalDevice() const { return devices->logicalDevice; }
	
	
	// ----- Misc -----
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
	//void CreateFramebuffers(const VkDevice &logicalDevice, const VkRenderPass &renderPass, uint32_t imageCount, uint32_t extentWidth, uint32_t extentHeight, std::vectorVkImageView **attachmentPtrs, const uint32_t &attachmentCount);
	bool framebufferResized = false;
	
//	void CreateAndFillDeviceLocalBuffer(VkBuffer &bufferHandle, VmaAllocation &allocation, void *data, const VkDeviceSize &size, const VkBufferUsageFlags &usageFlags);
//	void FillExistingDeviceLocalBuffer(VkBuffer bufferHandle, void *data, const VkDeviceSize &size);
	
	
	// semaphores and fences for each flying frame
	VkSemaphore imageAvailableSemaphoresFlying[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphoresFlying[MAX_FRAMES_IN_FLIGHT];
	VkFence inFlightFencesFlying[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore computeFinishedSemaphoresFlying[MAX_FRAMES_IN_FLIGHT];
	VkFence computeInFlightFencesFlying[MAX_FRAMES_IN_FLIGHT];
	
	// flying frames
	uint32_t currentFrameImageIndex;
	uint32_t currentFrame = 0;
	
	// ----- structures -----
//	std::vector<std::optional<UniformBufferObject>> uniformBufferObjects;
//	std::vector<std::optional<StorageBufferObject>> storageBufferObjects;
//	std::vector<std::optional<TextureImage>> textureImages;
//	std::vector<VkSampler> textureSamplers;
//	std::vector<std::optional<BufferedRenderPass>> bufferedRenderPasses;
//	std::vector<std::optional<LayeredBufferedRenderPass>> layeredBufferedRenderPasses;
	
	// vertex / index buffers
//	std::vector<std::optional<VertexBufferObject>> vertexBufferObjects;
//	std::vector<std::optional<IndexBufferObject>> indexBufferObjects;
	
//	std::tuple<shaderProgramTs...> shaderPrograms;
	
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
	
//	VkCommandBuffer BeginSingleTimeCommands();
//	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	
//	void CreateImage(const VkImageCreateInfo &imageCI, VkMemoryPropertyFlags properties, VkImage &image, VmaAllocation &allocation);
//	VkImageView CreateImageView(const VkImageViewCreateInfo &imageViewCI);
//	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);
//	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	
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
	
//	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VmaAllocation &allocation, VmaAllocationInfo *allocationInfoDst=nullptr);
//	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
//	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth=1);
	
//	void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
};


} // namespace EVK
