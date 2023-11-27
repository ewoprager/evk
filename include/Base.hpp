#ifndef EVK_hpp
#define EVK_hpp

//#define MSAA
//#define MAKE_ASSERTIONS

#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <ESDL_EventHandler.hpp>
#include <mat4x4/mat4x4.hpp>
#include <SDL2/SDL_image.h>

#define MAX_FRAMES_IN_FLIGHT 2

/*
 Initialisation:
	- `SDL_Init(...)`
	- `SDL_CreateWindow(..., SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | ...)`
	- Create an instance of `EVK::Devices`
	- Create an array of `EVK::ImageBlueprint`s, the indices of the `EVK::ImageBlueprint`s in this array will correspond to the indices of the resulting corresponding image structures created in the `EVK::Interface` instance, which are the indices used in image descriptors.
	- Create an instance of `EVL::Interface`
	- Fill required vertex and index buffers with `Vulkan::FillVertexBuffer` / `Vulkan::FillIndexBuffer`
 
 In the render loop:
	- (Access render pipelines with `EVK::Interface::RP(...)`, and pipeline descriptor sets with `EVK::Interface::RenderPipeline::DS(...)`)
 
	- Set the values of your uniforms by modifying the values pointed to by that returned/filled by `EVK::Interface::GetUniformBufferObjectPointer<<UBO struct type>>(...)` / `EVK::Interface::GetUniformBufferObjectPointers<<UBO struct type>>(...)`
	- Put your render pass in the following condition: `if(<EVK::Interface instance>.BeginFrame()){ ... }`
	- For each render pass:
		- Begin it with `EVK::Interface::CmdBegin(Layered)BufferedRenderPass(...)`, or `EVK::Interface::BeginFinalRenderPass()` for the final one
		- For each render pipeline:
			- Bind with `EVK::Interface::RenderPipeline::Bind()`
			Then, as many times as you want:
				- Set the desired descriptor set bindings with `EVK::Interface::RenderPipeline::BindDescriptorSets(...)` (this includes setting dynamic UBO offsets)
				- Set the desired push constant values with `EVK::Interface::RenderPipeline::CmdPushConstants<<PC struct type>>(...)`
				- Queue a draw call command `EVK::Interface::CmdDraw(...)` or `EVK::Interface::CmdDrawIndexed(...)`
		- Finish the pass with `EVK::Interface::CmdEndRenderPass()`, or `EVK::Interface::EndFinalRenderPassAndFrame()` for the final one
 
 Cleaning up:
	- `vkDeviceWaitIdle(Vulkan::GetLogicalDevice())`
	- Destroy your `EVK::Interface` instance
	- `SDL_DestroyWindow(<your SDL_Window pointer>)`
 */

namespace EVK {

class Interface;
class Devices;

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

//VkFormat SDLPixelFormatToVulkanFormat(const SDL_PixelFormatEnum &sdlPixelFormat);

struct UniformBufferObject {
	UniformBufferObject(){
		dynamicRepeats = 1;
	}
	
	VkBuffer buffersFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation allocationsFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo allocationInfosFlying[MAX_FRAMES_IN_FLIGHT];
	VkDeviceSize size;
	int dynamicRepeats; // not a dynamic UBO unless this is more than 1
	VkDeviceSize dynamicAlignment;
	
	void CleanUp(const VmaAllocator &allocator){
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) vmaDestroyBuffer(allocator, buffersFlying[i], allocationsFlying[i]);
	}
};
struct StorageBufferObject {
	StorageBufferObject(){}
	
	VkBuffer buffersFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation allocationsFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo allocationInfosFlying[MAX_FRAMES_IN_FLIGHT];
	VkDeviceSize size;
	
	void CleanUp(const VmaAllocator &allocator){
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) vmaDestroyBuffer(allocator, buffersFlying[i], allocationsFlying[i]);
	}
};
struct TextureImage {
	TextureImage(){}
	
	VkImage image;
	VmaAllocation allocation;
	VkImageView view;
	uint32_t mipLevels;
	
	void CleanUp(const VkDevice &logicalDevice, const VmaAllocator &allocator){
		vkDestroyImageView(logicalDevice, view, nullptr);
		vmaDestroyImage(allocator, image, allocation);
	}
};
struct BufferedRenderPass {
	BufferedRenderPass(){}
	
	uint32_t width, height;
	VkRenderPass renderPass;
	VkFramebuffer frameBuffersFlying[MAX_FRAMES_IN_FLIGHT];
	
	void CleanUp(const VkDevice &logicalDevice){
		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) vkDestroyFramebuffer(logicalDevice, frameBuffersFlying[i], nullptr);
	}
};
struct LayeredBufferedRenderPass {
	LayeredBufferedRenderPass(const int &_layersN) : layersN(_layersN){
		layerImageViews = (VkImageView *)malloc(_layersN*sizeof(VkImageView));
		frameBuffersFlying = (VkFramebuffer *)malloc(MAX_FRAMES_IN_FLIGHT*_layersN*sizeof(VkFramebuffer));
	}
	
	uint32_t width, height;
	int layersN;
	VkRenderPass renderPass;
	VkImageView *layerImageViews;
	VkFramebuffer *frameBuffersFlying;
	
	void CleanUp(const VkDevice &logicalDevice){
		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
		for(int i=0; i<layersN; i++) vkDestroyImageView(logicalDevice, layerImageViews[i], nullptr);
		free(layerImageViews);
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT*layersN; i++) vkDestroyFramebuffer(logicalDevice, frameBuffersFlying[i], nullptr);
		free(frameBuffersFlying);
	}
};

class Devices {
public:
	Devices(SDL_Window *const &_sdlWindowPtr);
	
	// shader modules
	VkShaderModule CreateShaderModule(const char *const &filename) const;
	
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
	VkFormat FindSupportedFormat(const VkFormat *const &candidates, const int &n, VkImageTiling tiling, VkFormatFeatureFlags features) const;
	VkFormat FindDepthFormat() const;
	
	SwapChainSupportDetails QuerySwapChainSupport() const;
	
private:
	SDL_Window *sdlWindowPtr;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	QueueFamilyIndices queueFamilyIndices;
	VkDevice logicalDevice;
	VmaAllocator allocator;
	
	// queues:
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
	
#ifdef MSAA
	// multi-sampled anti-aliasing (MSAA):
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT; // default to 1 sample per pixel (no multi-sampling)
	VkSampleCountFlagBits GetMaxUsableSampleCount() const;
#endif
	
	friend class Interface;
};

struct ImageBlueprint {
private:
	virtual void Build(int index, Interface &interface){}
	
	friend class Interface;
};
struct PNGImageBlueprint : public ImageBlueprint {
	PNGImageBlueprint(const char *_imageFilename) : imageFilename(strdup(_imageFilename)) {}
	~PNGImageBlueprint(){
		free(imageFilename);
	}
	
private:
	char *imageFilename;
	
	void Build(int index, Interface &interface) override;
	
	friend class Interface;
};
struct CubemapPNGImageBlueprint : public ImageBlueprint {
	CubemapPNGImageBlueprint(const char *_imageFilenames[6]){
		for(int i=0; i<6; i++) imageFilenames[i] = strdup(_imageFilenames[i]);
	}
	~CubemapPNGImageBlueprint(){
		for(int i=0; i<6; i++) free(imageFilenames[i]);
	}
	
private:
	char *imageFilenames[6];
	
	void Build(int index, Interface &interface) override;
	
	friend class Interface;
};
struct ManualImageBlueprint : public ImageBlueprint {
	ManualImageBlueprint(const VkImageCreateInfo &_imageCI, const VkImageViewType &_imageViewType, const VkMemoryPropertyFlags &_properties, const VkImageAspectFlags &_aspectFlags) : imageCI(_imageCI), imageViewType(_imageViewType), properties(_properties), aspectFlags(_aspectFlags) {}
	
private:
	VkImageCreateInfo imageCI;
	VkImageViewType imageViewType;
	VkMemoryPropertyFlags properties;
	VkImageAspectFlags aspectFlags;
	
	void Build(int index, Interface &interface) override;
	
	friend class Interface;
};
enum class DescriptorType {
	UBO,
	SBO,
	textureImage,
	textureSampler,
	combinedImageSampler,
	storageImage
};
struct DescriptorBlueprint {
	DescriptorType type;
	uint32_t binding;
	VkShaderStageFlags stageFlags;
	int count;
	int *indicesA;
	int *indicesB;
};
struct DescriptorSetBlueprint {
	int descriptorsN;
	DescriptorBlueprint *descriptorBlueprints;
};
struct PipelineBlueprint {
	int descriptorSetsN;
	DescriptorSetBlueprint *descriptorSetBlueprints;
	
	int pushConstantRangesN;
	VkPushConstantRange *pushConstantRanges;
};
struct GraphicsPipelineBlueprint {
	PipelineBlueprint pipelineBlueprint;
	
	uint32_t stageCount;
	VkPipelineShaderStageCreateInfo *shaderStageCIs;
	VkPipelineVertexInputStateCreateInfo vertexInputStateCI;
	VkPipelineRasterizationStateCreateInfo rasterisationStateCI;
	VkPipelineMultisampleStateCreateInfo multisampleStateCI;
	VkPipelineColorBlendStateCreateInfo colourBlendStateCI;
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI;
	VkPipelineDynamicStateCreateInfo dynamicStateCI;
	int bufferedRenderPassIndex; //=-1
	int layeredBufferedRenderPassIndex; //=-1
};
struct ComputePipelineBlueprint {
	PipelineBlueprint pipelineBlueprint;
	
	VkPipelineShaderStageCreateInfo shaderStageCI;
};
struct UniformBufferObjectBlueprint {
	VkDeviceSize size;
	int dynamicRepeats; //=1
};
struct StorageBufferObjectBlueprint {
	VkDeviceSize size;
	VkBufferUsageFlags usages;
	VkMemoryPropertyFlags memoryProperties;
};
struct BufferedRenderPassBlueprint {
	VkRenderPassCreateInfo renderPassCI;
	int textureImagesN;
	int *targetTextureImageIndices;
	uint32_t width; // if either with or height is 0, the buffer resizes with the window
	uint32_t height; //
};
struct LayeredBufferedRenderPassBlueprint {
	VkRenderPassCreateInfo renderPassCI;
	int targetTextureImageIndex;
	uint32_t width;
	uint32_t height;
	int layersN;
	VkFormat imageFormat;
	VkImageAspectFlags imageAspectFlags;
};
struct InterfaceBlueprint {
	Devices devices;
	
	int uniformBufferObjectsN;
	UniformBufferObjectBlueprint *uboBlueprints;
	
	int storageBufferObjectsN;
	StorageBufferObjectBlueprint *sboBlueprints;
	
	int textureSamplersN;
	VkSamplerCreateInfo *samplerBlueprints;
	
	int bufferedRenderPassesN;
	BufferedRenderPassBlueprint *bufferedRenderPassBlueprints;
	
	int layeredBufferedRenderPassesN;
	LayeredBufferedRenderPassBlueprint *layeredBufferedRenderPassBlueprints;
	
	int graphicsPipelinesN;
	GraphicsPipelineBlueprint *graphicsPipelineBlueprints;
	
	int computePipelinesN;
	ComputePipelineBlueprint *computePipelineBlueprints;
	
	int textureImagesN;
	ImageBlueprint **imageBlueprintPtrs;
	
	int vertexBuffersN;
	int indexBuffersN;
};
class Interface {
	friend class PNGImageBlueprint;
	friend class CubemapPNGImageBlueprint;
	friend class ManualImageBlueprint;
	class Pipeline;
	class GraphicsPipeline;
	class ComputePipeline;
public:
	Interface(const InterfaceBlueprint &info);
	~Interface();
	
	// -----------------
	// ----- Setup -----
	// -----------------
	
	// ----- Filling buffers -----
	void FillVertexBuffer(const int &vertexBufferIndex, void *const &vertices, const VkDeviceSize &size, const VkDeviceSize &offset=0);
	void FillIndexBuffer(const int &indexBufferIndex, uint32_t *const &indices, const size_t &indexCount, const VkDeviceSize &offset=0);
	
	
	// ----- Modifying UBO data -----
	template <typename T> T *GetUniformBufferObjectPointer(const int &uniformBufferObjectIndex) const {
		return (T *)uniformBufferObjects[uniformBufferObjectIndex].allocationInfosFlying[currentFrame].pMappedData;
	}
	template <typename T> void GetUniformBufferObjectPointers(const int &uniformBufferObjectIndex, T **out) const {
		if(uniformBufferObjects[uniformBufferObjectIndex].dynamicRepeats > 1){
			uint8_t *start = (uint8_t *)uniformBufferObjects[uniformBufferObjectIndex].allocationInfosFlying[currentFrame].pMappedData;
			for(int i=0; i<uniformBufferObjects[uniformBufferObjectIndex].dynamicRepeats; i++){
				out[i] = (T *)start;
				start += uniformBufferObjects[uniformBufferObjectIndex].dynamicAlignment;
			}
		} else out[0] = (T *)uniformBufferObjects[uniformBufferObjectIndex].allocationInfosFlying[currentFrame].pMappedData;
	}
	
	
	// ---------------------
	// ----- Pipelines -----
	// ---------------------
	GraphicsPipeline &GP(const int &index) const { return *graphicsPipelines[index]; }
	ComputePipeline &CP(const int &index) const { return *computePipelines[index]; }
	
	
	// ------------------------------------
	// ----- Frames and render passes -----
	// ------------------------------------
	bool BeginFrame();
	//void CmdBeginRenderPass(const VkRenderPassBeginInfo *const &renderPassBeginInfo, const VkSubpassContents &subpassContents);
	void CmdBeginBufferedRenderPass(const int &bufferedRenderPassIndex, const VkSubpassContents &subpassContents, int clearValueCount, const VkClearValue *const &clearValues);
	void CmdBeginLayeredBufferedRenderPass(const int &layeredBufferedRenderPassIndex, const VkSubpassContents &subpassContents, int clearValueCount, const VkClearValue *const &clearValues, const int &layer);
	void CmdEndRenderPass();
	void BeginFinalRenderPass();
	// ----- Render pass commands -----
	void CmdBindVertexBuffer(const uint32_t &binding, const int &index);
	void CmdBindIndexBuffer(const int &index, const VkIndexType &type);
	void CmdDraw(const uint32_t &vertexCount, const uint32_t &instanceCount=1, const uint32_t &firstVertex=0, const uint32_t &firstInstance=0);
	void CmdDrawIndexed(const uint32_t &indexCount, const uint32_t &instanceCount=1, const uint32_t &firstIndex=0, const int32_t &vertexOffset=0, const uint32_t &firstInstance=0);
	void CmdSetDepthBias(const float &constantFactor, const float &clamp, const float &slopeFactor);
	void CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
	
	void EndFinalRenderPassAndFrame();
	
	
	// ----- Getters -----
	const uint32_t &GetExtentWidth() const { return swapChainExtent.width; }
	const uint32_t &GetExtentHeight() const { return swapChainExtent.height; }
	const bool &GetVertexBufferCreated(const int &index) const { return vertexBuffersCreated[index]; }
	const bool &GetIndexBufferCreated(const int &index) const { return indexBuffersCreated[index]; }
	const VkDeviceSize &GetUniformBufferObjectDynamicAlignment(const int &index) const { return uniformBufferObjects[index].dynamicAlignment; }
	const int &GetUniformBufferObjectCount(){ return info.uniformBufferObjectsN; }
	const uint32_t &GetIndexBufferCount(const int &index) const { return indexBufferCounts[index]; }
	const VkRenderPass &GetBufferedRenderPassHandle(const int &index) const { return bufferedRenderPasses[index].renderPass; }
	const VkRenderPass &GetLayeredBufferedRenderPassHandle(const int &index) const { return layeredBufferedRenderPasses[index].renderPass; }
	const VkDevice &GetLogicalDevice() const { return devices.logicalDevice; }
	
	
	// ----- Misc -----
	void FramebufferResizeCallback(SDL_Event event){ framebufferResized = true; }
	
private:
	struct {
		SDL_Window *sdlWindowPtr;
		int uniformBufferObjectsN;
		int storageBufferObjectsN;
		int textureImagesN;
		int textureSamplersN;
		int bufferedRenderPassesN;
		int layeredBufferedRenderPassesN;
		int vertexBuffersN;
		int indexBuffersN;
		int graphicsPipelinesN;
		int computePipelinesN;
	} info;
	
	Devices devices;
	
	VkSwapchainKHR swapChain;
	
	VkImage *swapChainImages;
	VkImageView *swapChainImageViews;
	VkFramebuffer *swapChainFramebuffers;
	
	struct ResisingBRP {
		int index;
		BufferedRenderPassBlueprint blueprint;
	};
	struct ResisingImage {
		int index;
		ManualImageBlueprint blueprint;
	};
	int resisingBRPsN;
	ResisingBRP *resisingBRPs;
	int resisingImagesN;
	ResisingImage *resisingImages;
	
	uint32_t imageCount;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkCommandPool commandPool;
	VkRenderPass renderPass;
	void CreateFramebuffers(const VkDevice &logicalDevice, const VkRenderPass &renderPass, const uint32_t &imageCount, const uint32_t &extentWidth, const uint32_t &extentHeight, VkImageView **attachmentPtrs, const uint32_t &attachmentCount);
	bool framebufferResized = false;
	
	void FillDeviceLocalBuffer(VkBuffer &bufferHandle, VmaAllocation &allocation, void *const &data, const VkDeviceSize &size, const VkDeviceSize &offset, const VkBufferUsageFlags &usageFlags);
	
	// a command buffer for each flying frame
	VkCommandBuffer commandBuffersFlying[MAX_FRAMES_IN_FLIGHT];
	
	// semaphores and fences for each flying frame
	VkSemaphore imageAvailableSemaphoresFlying[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphoresFlying[MAX_FRAMES_IN_FLIGHT];
	VkFence inFlightFencesFlying[MAX_FRAMES_IN_FLIGHT];
	
	// flying frames
	uint32_t currentFrameImageIndex;
	uint32_t currentFrame = 0;
	
	
	// ----- structures -----
	UniformBufferObject *uniformBufferObjects;
	void BuildUBO(int index, const UniformBufferObjectBlueprint &blueprint);
	
	StorageBufferObject *storageBufferObjects;
	void BuildSBO(int index, const StorageBufferObjectBlueprint &blueprint);
	
	TextureImage *textureImages;
	void BuildTextureImageFromFile(int index, const PNGImageBlueprint &blueprint);
	void BuildCubemapImageFromFiles(int index, const CubemapPNGImageBlueprint &blueprint);
	void BuildTextureImage(int index, ManualImageBlueprint blueprint);
	
	VkSampler *textureSamplers;
	void BuildTextureSampler(int index, const VkSamplerCreateInfo &samplerCI);
	
	BufferedRenderPass *bufferedRenderPasses;
	bool BuildBufferedRenderPass(int index, const BufferedRenderPassBlueprint &blueprint); // returns if it resises with the window
	
	LayeredBufferedRenderPass *layeredBufferedRenderPasses;
	void BuildLayeredBufferedRenderPass(int index, const LayeredBufferedRenderPassBlueprint &blueprint);
	
	// vertex buffers
	bool *vertexBuffersCreated;
	VkBuffer *vertexBufferHandles;
	VkDeviceSize *vertexBufferOffsets;
	VmaAllocation *vertexBufferAllocations;
	
	// index buffers
	bool *indexBuffersCreated;
	VkBuffer *indexBufferHandles;
	VkDeviceSize *indexBufferOffsets;
	VmaAllocation *indexBufferAllocations;
	uint32_t *indexBufferCounts;
	
	class Pipeline {
	protected:
		class DescriptorSet;
		
	public:
		// The pipline constructor initialises all the contained descriptor sets
		Pipeline(Interface &_vulkan, const PipelineBlueprint &blueprint);
		
		~Pipeline(){
			for(int i=0; i<descriptorSetsN; i++) delete descriptorSets[i];
			free(pushConstantRanges);
			free(descriptorSets);
			free(descriptorSetLayouts);
			free(descriptorSetsFlying);
			vkDestroyDescriptorPool(vulkan.devices.logicalDevice, descriptorPool, nullptr);
			vkDestroyPipeline(vulkan.devices.logicalDevice, pipeline, nullptr);
			vkDestroyPipelineLayout(vulkan.devices.logicalDevice, layout, nullptr);
			//vkDestroyShaderModule(vulkan.devices.logicalDevice, fragShaderModule, nullptr);
			//vkDestroyShaderModule(vulkan.devices.logicalDevice, vertShaderModule, nullptr);
		}
		
		// ----- Methods to call after Init() -----
		// Bind the pipeline for subsequent render calls
		virtual void Bind(){}
		// Set which descriptor sets are bound for subsequent render calls
		virtual void BindDescriptorSets(const int &first, const int &number, const uint32_t &dynamicOffsetCount=0, const int *const &dynamicOffsetNumbers=nullptr){}
		void UpdateDescriptorSets(); // have to do this every time any elements of any descriptors are changed, e.g. when an image view is re-created upon window resize
		// Set push constant data
		template <typename T> void CmdPushConstants(const int &index, T *const &data){
#ifdef MAKE_ASSERTIONS
			assert(pushConstantRanges[index].size == sizeof(T));
#endif
			vkCmdPushConstants(vulkan.commandBuffersFlying[vulkan.currentFrame], layout, pushConstantRanges[index].stageFlags, pushConstantRanges[index].offset, pushConstantRanges[index].size, data);
		}
		
		// Get the handle of a descriptor set
		DescriptorSet &DS(const int &index){ return *descriptorSets[index]; }
		
	protected:
		Interface &vulkan;
		
		class DescriptorSet {
			class Descriptor;
		public:
			DescriptorSet(Pipeline &_pipeline, int _index, const DescriptorSetBlueprint &blueprint);
			~DescriptorSet(){
				for(int i=0; i<descriptorsN; i++) delete descriptors[i];
				vkDestroyDescriptorSetLayout(pipeline.vulkan.devices.logicalDevice, pipeline.descriptorSetLayouts[index], nullptr);
			}
			
			// For initialisation
			void InitLayouts();
			void InitConfigurations();
			
			const int &GetDescriptorCount() const { return descriptorsN; }
			const Descriptor *const &GetDescriptor(const int &index){ return descriptors[index]; }
			const bool &GetUBODynamic() const { return uboDynamic; }
			const VkDeviceSize &GetUBODynamicAlignment() const { return uboDynamicAlignment; }
			
		private:
			Pipeline &pipeline;
			int index;
			int descriptorsN = 0;
			Descriptor **descriptors;
			
			// ubo info
			bool uboDynamic = false;
			VkDeviceSize uboDynamicAlignment;
			/*
			 `index` is the index of this descriptor set's layout in 'Vulkan::RenderPipeline::descriptorSetLayouts',
			 
			 `pipeline.descriptorSetNumber*flightIndex`,
			 `pipeline.descriptorSetNumber*flightIndex + 1`
			 ...
			 `pipeline.descriptorSetNumber*flightIndex + pipeline.descriptorSetNumber - 1`
			 are the indices of this descriptors flying sets in 'pipeline.descriptorSetsFlying'
			 */
			
			class Descriptor {
			public:
				Descriptor(DescriptorSet &_descriptorSet, const uint32_t &_binding, const VkShaderStageFlags &_stageFlags) : descriptorSet(_descriptorSet), binding(_binding), stageFlags(_stageFlags) {}
				virtual ~Descriptor(){}
				
				virtual VkDescriptorSetLayoutBinding LayoutBinding() const = 0;
				
				virtual VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const = 0;
				
				virtual VkDescriptorPoolSize PoolSize() const = 0;
				
			protected:
				DescriptorSet &descriptorSet;
				uint32_t binding;
				VkShaderStageFlags stageFlags;
			};
			class UBODescriptor : public Descriptor {
			public:
				UBODescriptor(DescriptorSet &_descriptorSet, const uint32_t &_binding, const VkShaderStageFlags &_stageFlags, const int &_index) : Descriptor(_descriptorSet, _binding, _stageFlags), index(_index) {}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				int index;
			};
			class SBODescriptor : public Descriptor {
			public:
				SBODescriptor(DescriptorSet &_descriptorSet, const uint32_t &_binding, const VkShaderStageFlags &_stageFlags, const int &_index) : Descriptor(_descriptorSet, _binding, _stageFlags), index(_index) {}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				int index;
			};
			class TextureImagesDescriptor : public Descriptor {
			public:
				TextureImagesDescriptor(DescriptorSet &_descriptorSet, const uint32_t &_binding, const VkShaderStageFlags &_stageFlags, const int &_count, const int *const &_indices) : Descriptor(_descriptorSet, _binding, _stageFlags), count(_count) {
					indices = (int *)malloc(_count*sizeof(int));
					memcpy(indices, _indices, _count*sizeof(int));
				}
				~TextureImagesDescriptor(){
					free(indices);
				}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				int count;
				int *indices;
			};
			class TextureSamplersDescriptor : public Descriptor {
			public:
				TextureSamplersDescriptor(DescriptorSet &_descriptorSet, const uint32_t &_binding, const VkShaderStageFlags &_stageFlags, const int &_count, const int *const &_indices) : Descriptor(_descriptorSet, _binding, _stageFlags), count(_count) {
					indices = (int *)malloc(_count*sizeof(int));
					memcpy(indices, _indices, _count*sizeof(int));
				}
				~TextureSamplersDescriptor(){
					free(indices);
				}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				int count;
				int *indices;
			};
			class CombinedImageSamplersDescriptor : public Descriptor {
			public:
				CombinedImageSamplersDescriptor(DescriptorSet &_descriptorSet, const uint32_t &_binding, const VkShaderStageFlags &_stageFlags, const int &_count, const int *const &_textureImageIndices, const int *const &_samplerIndices) : Descriptor(_descriptorSet, _binding, _stageFlags), count(_count) {
					textureImageIndices = (int *)malloc(_count*sizeof(int));
					samplerIndices = (int *)malloc(_count*sizeof(int));
					memcpy(textureImageIndices, _textureImageIndices, _count*sizeof(int));
					memcpy(samplerIndices, _samplerIndices, _count*sizeof(int));
				}
				~CombinedImageSamplersDescriptor(){
					free(textureImageIndices);
					free(samplerIndices);
				}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				int count;
				int *textureImageIndices;
				int *samplerIndices;
			};
			class StorageImagesDescriptor : public Descriptor {
			public:
				StorageImagesDescriptor(DescriptorSet &_descriptorSet, const uint32_t &_binding, const VkShaderStageFlags &_stageFlags, const int &_count, const int *const &_indices) : Descriptor(_descriptorSet, _binding, _stageFlags), count(_count) {
					indices = (int *)malloc(_count*sizeof(int));
					memcpy(indices, _indices, _count*sizeof(int));
				}
				~StorageImagesDescriptor(){
					free(indices);
				}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				int count;
				int *indices;
			};
		};
		DescriptorSet **descriptorSets;
		int descriptorSetsN;
		VkDescriptorSetLayout *descriptorSetLayouts;
		VkDescriptorSet *descriptorSetsFlying;
		VkDescriptorPool descriptorPool;
		
		VkPushConstantRange *pushConstantRanges;
		int pushConstantRangesN;
		
		//VkShaderModule vertShaderModule;
		//VkShaderModule fragShaderModule;
		VkPipelineLayout layout;
		VkPipeline pipeline;
	};
	class GraphicsPipeline : public Pipeline {
	public:
		GraphicsPipeline(Interface &_vulkan, const GraphicsPipelineBlueprint &blueprint);
		~GraphicsPipeline(){
			vkDestroyShaderModule(vulkan.devices.logicalDevice, fragShaderModule, nullptr);
			vkDestroyShaderModule(vulkan.devices.logicalDevice, vertShaderModule, nullptr);
		}
		
		void Bind() override;
		void BindDescriptorSets(const int &first, const int &number, const uint32_t &dynamicOffsetCount=0, const int *const &dynamicOffsetNumbers=nullptr) override;
		
	private:
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
	GraphicsPipeline **graphicsPipelines;
	
	class ComputePipeline : public Pipeline {
	public:
		ComputePipeline(Interface &_vulkan, const ComputePipelineBlueprint &blueprint);
		~ComputePipeline(){
			vkDestroyShaderModule(vulkan.devices.logicalDevice, shaderModule, nullptr);
		}
		
		void Bind() override;
		void BindDescriptorSets(const int &first, const int &number, const uint32_t &dynamicOffsetCount=0, const int *const &dynamicOffsetNumbers=nullptr) override;
		
	private:
		VkShaderModule shaderModule;
	};
	ComputePipeline **computePipelines;
	
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
	
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	
	void CreateImage(const VkImageCreateInfo &imageCI, VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation& allocation);
	VkImageView CreateImageView(const VkImageViewCreateInfo &imageViewCI);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	
	void CreateSwapChain();
	void CreateImageViews();
	void CreateColourResources();
	void CreateDepthResources();
	void CreateFramebuffers();
	void CleanUpSwapChain();
	void RecreateResisingBRPsAndImages();
	void RecreateSwapChain(){
		// in case we are minimised:
		int width, height;
		SDL_GL_GetDrawableSize(info.sdlWindowPtr, &width, &height);
		while(width == 0 || height == 0){
			SDL_GL_GetDrawableSize(info.sdlWindowPtr, &width, &height);
			SDL_WaitEvent(nullptr);
		}
		
		vkDeviceWaitIdle(devices.logicalDevice);
		
		CleanUpSwapChain();
		
		CreateSwapChain();
		CreateImageViews();
#ifdef MSAA
		CreateColourResources();
#endif
		CreateDepthResources();
		CreateFramebuffers();
		
		RecreateResisingBRPsAndImages();
	}
	
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo *const &allocationInfoDst=nullptr);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	
	void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
};


} // namespace::EVK


#endif /* EVK_hpp */
