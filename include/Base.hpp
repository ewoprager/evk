#ifndef EVK_hpp
#define EVK_hpp

//#define MSAA

#include <iostream>
#include <assert.h>
#include <chrono>
#include <thread>
#include <functional>

#include <MoltenVK/mvk_vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <SDL2/SDL_image.h>

#define MAX_FRAMES_IN_FLIGHT 2

#define VULKAN_VERSION_USING VK_API_VERSION_1_2

/*
 !!! BEWARE !!!
	 Be careful about the scopes of large blueprints that contain Vulkan 'create info's of various kinds.
	 If a Vulkan 'create info' has a pointer to some stuff, that pointer must remain valid at the point
	 of use of the outer-most blueprint!
 */

/*
 Initialisation:
	\/ For with SDL
	- `SDL_Init(...)`
	- `SDL_CreateWindow(..., SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | ...)`
	/\
	- Create an instance of `EVK::Devices`
		\/ For with SDL
		- Surface creation function:
`[window](VkInstance instance) -> VkSurfaceKHR {
	VkSurfaceKHR ret;
	if(SDL_Vulkan_CreateSurface(window, instance, &ret) == SDL_FALSE)
		throw std::runtime_error("failed to create window devices.surface!");
	return ret;
}`
		- Extent getter function:
 `[window]() -> VkExtent2D {
	 int width, height;
	 SDL_GL_GetDrawableSize(window, &width, &height);
	 return (VkExtent2D){
		 .width = uint32_t(width),
		 .height = uint32_t(height)
	 };
 }`
		/\
	- Create an instance of `EVK::Interface`
	- Add a window resize event callback to call method `EVK::Interface::FramebufferResizeCallback()` of the interface
		\/ For with SDL
		-   SDL_Event event;
			event.type = SDL_WINDOWEVENT;
			event.window.event = SDL_WINDOWEVENT_SIZE_CHANGED;
		/\
		\/ For with wxWidgets
		- Event type: `wxEVT_SIZE`
		/\
	- Fill required vertex and index buffers with `Vulkan::FillVertexBuffer` / `Vulkan::FillIndexBuffer`
	- Build structuers like UBOs, textures and buffered render passes
	- Build pipelines
 
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
	- `vkDeviceWaitIdle(EVK::Interface::GetLogicalDevice())`
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

struct VertexBufferObject {
	VkBuffer bufferHandle;
	VkDeviceSize offset;
	VmaAllocation allocation;
	VkDeviceSize size;
	
	void CleanUp(const VmaAllocator &allocator){
		vmaDestroyBuffer(allocator, bufferHandle, allocation);
	}
};
struct IndexBufferObject {
	VkBuffer bufferHandle;
	VkDeviceSize offset;
	VmaAllocation allocation;
	uint32_t indexCount;
	
	void CleanUp(const VmaAllocator &allocator){
		vmaDestroyBuffer(allocator, bufferHandle, allocation);
	}
};

struct UniformBufferObject {
	VkBuffer buffersFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation allocationsFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo allocationInfosFlying[MAX_FRAMES_IN_FLIGHT];
	VkDeviceSize size;
	
	struct Dynamic {
		int repeatsN;
		VkDeviceSize alignment;
	};
	std::optional<Dynamic> dynamic;
	
	void CleanUp(const VmaAllocator &allocator){
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) vmaDestroyBuffer(allocator, buffersFlying[i], allocationsFlying[i]);
	}
};
struct StorageBufferObject {
	VkBuffer buffersFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation allocationsFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo allocationInfosFlying[MAX_FRAMES_IN_FLIGHT];
	VkDeviceSize size;
	
	void CleanUp(const VmaAllocator &allocator){
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) vmaDestroyBuffer(allocator, buffersFlying[i], allocationsFlying[i]);
	}
};

struct PNGImageBlueprint {
	std::string imageFilename;
	
};
struct DataImageBlueprint {
	uint8_t *data;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	VkFormat format;
	bool mip;
};
struct CubemapPNGImageBlueprint {
	std::array<std::string, 6> imageFilenames;
};
struct ManualImageBlueprint {
	VkImageCreateInfo imageCI; // is this safe? given that we pass these around
	VkImageViewType imageViewType;
	VkMemoryPropertyFlags properties;
	VkImageAspectFlags aspectFlags;
};
struct TextureImage {
	VkImage image;
	VmaAllocation allocation;
	VkImageView view;
	uint32_t mipLevels;
	ManualImageBlueprint blueprint;
	
	void CleanUp(const VkDevice &logicalDevice, const VmaAllocator &allocator){
		vkDestroyImageView(logicalDevice, view, nullptr);
		vmaDestroyImage(allocator, image, allocation);
	}
};
struct BufferedRenderPass {
	uint32_t width, height;
	VkRenderPass renderPass;
	std::vector<int> targetTextureImageIndices;
	VkFramebuffer frameBuffersFlying[MAX_FRAMES_IN_FLIGHT];
	bool resising;
	
	void CleanUp(const VkDevice &logicalDevice){
		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) vkDestroyFramebuffer(logicalDevice, frameBuffersFlying[i], nullptr);
	}
};
struct LayeredBufferedRenderPass {
	LayeredBufferedRenderPass(int layersN) {
		layers = std::vector<Layer>(layersN);
	}
	
	uint32_t width, height;
	VkRenderPass renderPass;
	int targetTextureImageIndex;
	VkFormat imageFormat;
	VkImageAspectFlags imageAspectFlags;
	
	struct Layer {
		VkImageView imageView;
		VkFramebuffer frameBuffersFlying[MAX_FRAMES_IN_FLIGHT];
		
		void CleanUp(const VkDevice &logicalDevice){
			vkDestroyImageView(logicalDevice, imageView, nullptr);
			for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
				vkDestroyFramebuffer(logicalDevice, frameBuffersFlying[i], nullptr);
			}
		}
	};
	std::vector<Layer> layers;
	
	void CleanUp(const VkDevice &logicalDevice){
		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
		for(Layer &layer : layers) layer.CleanUp(logicalDevice);
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
	std::vector<int> indicesExtra;
	std::vector<int> indicesExtra2;
	/*
	 Usage of indicesExtra:
	 - UBO: {ubo index}
	 - SBO: {sbo index, flight offset}
	 - textureImage: {textureImage indices...}
	 - textureSampler: {textureSampler indices...}
	 - combinedImageSampler: {textureImage indices...}
	 - storageImage: {textureImage indices...}
	 Usage of indicesExtra2:
	 - combinedImageSampler: {textureSampler indices...}
	 */
};

using DescriptorSetBlueprint = std::vector<DescriptorBlueprint>;

struct PipelineBlueprint {
	std::vector<DescriptorSetBlueprint> descriptorSetBlueprints;
	
	std::vector<VkPushConstantRange> pushConstantRanges;
};
struct GraphicsPipelineBlueprint {
	PipelineBlueprint pipelineBlueprint;
	
	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCIs;
	VkPipelineVertexInputStateCreateInfo vertexInputStateCI;
	VkPrimitiveTopology primitiveTopology;
	VkPipelineRasterizationStateCreateInfo rasterisationStateCI;
	VkPipelineMultisampleStateCreateInfo multisampleStateCI;
	VkPipelineColorBlendStateCreateInfo colourBlendStateCI;
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI;
	VkPipelineDynamicStateCreateInfo dynamicStateCI;
	std::optional<int> bufferedRenderPassIndex;
	std::optional<int> layeredBufferedRenderPassIndex;
};
struct ComputePipelineBlueprint {
	PipelineBlueprint pipelineBlueprint;
	
	VkPipelineShaderStageCreateInfo shaderStageCI;
};
struct UniformBufferObjectBlueprint {
	VkDeviceSize size;
	std::optional<int> dynamicRepeats;
};
struct StorageBufferObjectBlueprint {
	VkDeviceSize size;
	VkBufferUsageFlags usages;
	VkMemoryPropertyFlags memoryProperties;
};
struct BufferedRenderPassBlueprint {
	VkRenderPassCreateInfo renderPassCI;
	std::vector<int> targetTextureImageIndices;
	
	// if either with or height is 0, the buffer resizes with the window
	uint32_t width, height;
};
struct LayeredBufferedRenderPassBlueprint {
	VkRenderPassCreateInfo renderPassCI;
	int targetTextureImageIndex;
	uint32_t width, height;
	int layersN;
	VkFormat imageFormat;
	VkImageAspectFlags imageAspectFlags;
};
struct InterfaceBlueprint {
	// removing copy and move constructors and assignment operators so it's harder to use the blueprint in a different scope
	InterfaceBlueprint(const Devices &_devices) : devices(_devices) {}
	
	const Devices &devices;
	
	int uniformBufferObjectsN;
	int storageBufferObjectsN;
	std::vector<VkSamplerCreateInfo> samplerBlueprints;
	int bufferedRenderPassesN;
	int layeredBufferedRenderPassesN;
	int graphicsPipelinesN;
	int computePipelinesN;
	int imagesN;
	int vertexBuffersN;
	int indexBuffersN;
};
class Interface {
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
	void FillVertexBuffer(int vertexBufferIndex, void *vertices, const VkDeviceSize &size, const VkDeviceSize &offset=0);
	void FillIndexBuffer(int indexBufferIndex, uint32_t *indices, size_t indexCount, const VkDeviceSize &offset=0);
	void FillStorageBuffer(int storageBufferIndex, void *data);
	
	
	// ----- Modifying UBO data -----
	template <typename T> T *GetUniformBufferObjectPointer(int uniformBufferObjectIndex) const {
		if(!uniformBufferObjects[uniformBufferObjectIndex])
			throw std::runtime_error("Cannot get UBO pointer; UBO not created");
		return (T *)uniformBufferObjects[uniformBufferObjectIndex]->allocationInfosFlying[currentFrame].pMappedData;
	}
	template <typename T> std::vector<T *> GetUniformBufferObjectPointers(int uniformBufferObjectIndex) const {
		if(!uniformBufferObjects[uniformBufferObjectIndex])
			throw std::runtime_error("Cannot get UBO pointers; UBO not created");
		std::vector<T *> ret {};
		if(uniformBufferObjects[uniformBufferObjectIndex]->dynamic){
			const int number = uniformBufferObjects[uniformBufferObjectIndex]->dynamic->repeatsN;
			ret.resize(number);
			uint8_t *start = (uint8_t *)uniformBufferObjects[uniformBufferObjectIndex]->allocationInfosFlying[currentFrame].pMappedData;
			for(T* &ptr : ret){
				ptr = (T *)start;
				start += uniformBufferObjects[uniformBufferObjectIndex]->dynamic->alignment;
			}
		} else {
			ret.push_back((T *)uniformBufferObjects[uniformBufferObjectIndex]->allocationInfosFlying[currentFrame].pMappedData);
		}
		return ret;
	}
	
	
	// ----- Structures -----
	void BuildUBO(int index, const UniformBufferObjectBlueprint &blueprint);
	void BuildSBO(int index, const StorageBufferObjectBlueprint &blueprint);
	void BuildTextureSampler(int index, const VkSamplerCreateInfo &samplerCI);
	void BuildBufferedRenderPass(int index, const BufferedRenderPassBlueprint &blueprint);
	void UpdateBufferedRenderPass(int index);
	void BuildLayeredBufferedRenderPass(int index, const LayeredBufferedRenderPassBlueprint &blueprint);
	void UpdateLayeredBufferedRenderPass(int index);
	
	
	// ---------------------
	// ----- Pipelines -----
	// ---------------------
	void BuildGraphicsPipeline(int index, const GraphicsPipelineBlueprint &blueprint);
	void BuildComputePipeline(int index, const ComputePipelineBlueprint &blueprint);
	GraphicsPipeline &GP(int index) const { return *graphicsPipelines[index]; }
	ComputePipeline &CP(int index) const { return *computePipelines[index]; }
	
	
	// ------------------------------------
	// ----- Frames and render passes -----
	// ------------------------------------
	// ----- General commands -----
	void CmdPipelineImageMemoryBarrier(bool graphicsOrCompute, int imageIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkImageSubresourceRange subresourceRange);
	// ----- Graphics commands -----
	bool BeginFrame();
	void CmdBeginBufferedRenderPass(int bufferedRenderPassIndex, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues);
	void CmdBeginLayeredBufferedRenderPass(int layeredBufferedRenderPassIndex, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues, int layer);
	void CmdEndRenderPass();
	void BeginFinalRenderPass(const VkClearColorValue &clearColour={{0.0f, 0.0f, 0.0f, 0.0f}});
	void EndFinalRenderPassAndFrame(std::optional<VkPipelineStageFlags> stagesWaitForCompute=std::optional<VkPipelineStageFlags>());
	// ----- Render pass commands -----
	void CmdBindVertexBuffer(uint32_t binding, int index);
	void CmdBindStorageBufferAsVertexBuffer(uint32_t binding, int index, const VkDeviceSize &offset=0);
	void CmdBindIndexBuffer(int index, const VkIndexType &type);
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
	void BuildTextureImageFromFile(int index, const PNGImageBlueprint &blueprint);
	void BuildCubemapImageFromFiles(int index, const CubemapPNGImageBlueprint &blueprint);
	void BuildTextureImage(int index, ManualImageBlueprint blueprint);
	void BuildDataImage(int index, const DataImageBlueprint &blueprint);
	
	
	// ----- Getters -----
	const uint32_t &GetExtentWidth() const { return swapChainExtent.width; }
	const uint32_t &GetExtentHeight() const { return swapChainExtent.height; }
	bool GetVertexBufferCreated(int index) const { return (bool)vertexBufferObjects[index]; }
	bool GetIndexBufferCreated(int index) const { return (bool)indexBufferObjects[index]; }
	std::optional<VkDeviceSize> GetUniformBufferObjectDynamicAlignment(int index) const { return uniformBufferObjects[index]->dynamic ? uniformBufferObjects[index]->dynamic.value().alignment : std::optional<VkDeviceSize>(); }
	size_t GetUniformBufferObjectCount(){ return uniformBufferObjects.size(); }
	uint32_t GetIndexBufferCount(int index) const {
		if(!indexBufferObjects[index]) return 0;
		return indexBufferObjects[index]->indexCount;
	}
	const VkRenderPass &GetBufferedRenderPassHandle(int index) const { return bufferedRenderPasses[index]->renderPass; }
	const VkRenderPass &GetLayeredBufferedRenderPassHandle(int index) const { return layeredBufferedRenderPasses[index]->renderPass; }
	const VkDevice &GetLogicalDevice() const { return devices.logicalDevice; }
	
	
	// ----- Misc -----
	void FramebufferResizeCallback(){ framebufferResized = true; }
	
	
	const Devices &devices;
	
private:
	VkSwapchainKHR swapChain;
	
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	
	std::vector<int> resisingBRPs;
	std::vector<int> resisingImages;
	
	uint32_t imageCount;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkCommandPool commandPool;
	VkRenderPass renderPass;
	//void CreateFramebuffers(const VkDevice &logicalDevice, const VkRenderPass &renderPass, uint32_t imageCount, uint32_t extentWidth, uint32_t extentHeight, std::vectorVkImageView **attachmentPtrs, const uint32_t &attachmentCount);
	bool framebufferResized = false;
	
	void CreateAndFillDeviceLocalBuffer(VkBuffer &bufferHandle, VmaAllocation &allocation, void *data, const VkDeviceSize &size, const VkBufferUsageFlags &usageFlags);
	void FillExistingDeviceLocalBuffer(VkBuffer bufferHandle, void *data, const VkDeviceSize &size);
	
	// a command buffer for each flying frame
	VkCommandBuffer commandBuffersFlying[MAX_FRAMES_IN_FLIGHT];
	VkCommandBuffer computeCommandBuffersFlying[MAX_FRAMES_IN_FLIGHT];
	
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
	std::vector<std::optional<UniformBufferObject>> uniformBufferObjects;
	std::vector<std::optional<StorageBufferObject>> storageBufferObjects;
	std::vector<std::optional<TextureImage>> textureImages;
	std::vector<VkSampler> textureSamplers;
	std::vector<std::optional<BufferedRenderPass>> bufferedRenderPasses;
	std::vector<std::optional<LayeredBufferedRenderPass>> layeredBufferedRenderPasses;
	
	// vertex / index buffers
	std::vector<std::optional<VertexBufferObject>> vertexBufferObjects;
	std::vector<std::optional<IndexBufferObject>> indexBufferObjects;
	
	class Pipeline {
	protected:
		class DescriptorSet;
		
	public:
		// The pipline constructor initialises all the contained descriptor sets
		Pipeline(Interface &_vulkan, const PipelineBlueprint &blueprint);
		
		~Pipeline(){
			vkDestroyDescriptorPool(vulkan.devices.logicalDevice, descriptorPool, nullptr);
			vkDestroyPipeline(vulkan.devices.logicalDevice, pipeline, nullptr);
			vkDestroyPipelineLayout(vulkan.devices.logicalDevice, layout, nullptr);
			for(VkDescriptorSetLayout &dsl : descriptorSetLayouts){
				vkDestroyDescriptorSetLayout(vulkan.devices.logicalDevice, dsl, nullptr);
			}
		}
		
		// ----- Methods to call after Init() -----
		// Bind the pipeline for subsequent render calls
		virtual void Bind() = 0;
		// Set which descriptor sets are bound for subsequent render calls
		virtual void BindDescriptorSets(int first, int number, const std::vector<int> &dynamicOffsetNumbers=std::vector<int>()) = 0;
		void UpdateDescriptorSets(uint32_t first=0); // have to do this every time any elements of any descriptors are changed, e.g. when an image view is re-created upon window resize
		// Set push constant data
		template <typename T> void CmdPushConstants(int index, T *data){
			assert(pushConstantRanges[index].size == sizeof(T));
			vkCmdPushConstants(vulkan.commandBuffersFlying[vulkan.currentFrame], layout, pushConstantRanges[index].stageFlags, pushConstantRanges[index].offset, pushConstantRanges[index].size, data);
		}
		
		// Get the handle of a descriptor set
		DescriptorSet &DS(int index){ return *descriptorSets[index]; }
		
	protected:
		Interface &vulkan;
		
		class DescriptorSet {
			class Descriptor;
		public:
			DescriptorSet(Pipeline &_pipeline, int _index, const DescriptorSetBlueprint &blueprint);
			~DescriptorSet(){}
			
			// For initialisation
			void InitLayouts();
			void Update();
			
			size_t GetDescriptorCount() const { return descriptors.size(); }
			std::shared_ptr<Descriptor> GetDescriptor(int index) const { return descriptors[index]; }
			bool GetUBODynamic() const { return uboDynamicAlignment.has_value(); }
			const std::optional<VkDeviceSize> &GetUBODynamicAlignment() const { return uboDynamicAlignment; }
			
		private:
			Pipeline &pipeline;
			int index;
			
			std::vector<std::shared_ptr<Descriptor>> descriptors;
			
			// ubo info
			std::optional<VkDeviceSize> uboDynamicAlignment;
			
			// this still relevent? \/\/\/
			/*
			 `index` is the index of this descriptor set's layout in 'Vulkan::RenderPipeline::descriptorSetLayouts',
			 
			 `pipeline.descriptorSetNumber*flightIndex`,
			 `pipeline.descriptorSetNumber*flightIndex + 1`
			 ...
			 `pipeline.descriptorSetNumber*flightIndex + pipeline.descriptorSetNumber - 1`
			 are the indices of this descriptors flying sets in 'pipeline.descriptorSetsFlying'
			 */
			// /\/\/\
			
			class Descriptor {
			public:
				Descriptor(DescriptorSet &_descriptorSet, uint32_t _binding, const VkShaderStageFlags &_stageFlags) : descriptorSet(_descriptorSet), binding(_binding), stageFlags(_stageFlags) {}
				virtual ~Descriptor(){}
				
				virtual VkDescriptorSetLayoutBinding LayoutBinding() const = 0;
				
				virtual VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const = 0;
				
				virtual VkDescriptorPoolSize PoolSize() const = 0;
				
			protected:
				DescriptorSet &descriptorSet;
				uint32_t binding;
				VkShaderStageFlags stageFlags;
			};
			class UBODescriptor : public Descriptor {
			public:
				UBODescriptor(DescriptorSet &_descriptorSet, uint32_t _binding, const VkShaderStageFlags &_stageFlags, int _index) : Descriptor(_descriptorSet, _binding, _stageFlags), index(_index) {}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				int index;
			};
			class SBODescriptor : public Descriptor {
			public:
				SBODescriptor(DescriptorSet &_descriptorSet, uint32_t _binding, const VkShaderStageFlags &_stageFlags, int _index, int _flightOffset) : Descriptor(_descriptorSet, _binding, _stageFlags), index(_index), flightOffset(_flightOffset) {}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				int index;
				int flightOffset;
			};
			class TextureImagesDescriptor : public Descriptor {
			public:
				TextureImagesDescriptor(DescriptorSet &_descriptorSet, uint32_t _binding, const VkShaderStageFlags &_stageFlags, const std::vector<int> &_indices) : Descriptor(_descriptorSet, _binding, _stageFlags), indices(_indices) {}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				std::vector<int> indices;
			};
			class TextureSamplersDescriptor : public Descriptor {
			public:
				TextureSamplersDescriptor(DescriptorSet &_descriptorSet, uint32_t _binding, const VkShaderStageFlags &_stageFlags, const std::vector<int> &_indices) : Descriptor(_descriptorSet, _binding, _stageFlags), indices(_indices) {}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				std::vector<int> indices;
			};
			class CombinedImageSamplersDescriptor : public Descriptor {
			public:
				CombinedImageSamplersDescriptor(DescriptorSet &_descriptorSet, uint32_t _binding, const VkShaderStageFlags &_stageFlags, const std::vector<int> &_textureImageIndices, const std::vector<int> &_samplerIndices) : Descriptor(_descriptorSet, _binding, _stageFlags), textureImageIndices(_textureImageIndices), samplerIndices(_samplerIndices) {
					assert(_textureImageIndices.size() == _samplerIndices.size());
				}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				std::vector<int> textureImageIndices;
				std::vector<int> samplerIndices;
			};
			class StorageImagesDescriptor : public Descriptor {
			public:
				StorageImagesDescriptor(DescriptorSet &_descriptorSet, uint32_t _binding, const VkShaderStageFlags &_stageFlags, const std::vector<int> &_indices) : Descriptor(_descriptorSet, _binding, _stageFlags), indices(_indices) {}
				
				VkDescriptorSetLayoutBinding LayoutBinding() const override;
				
				VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
				
				VkDescriptorPoolSize PoolSize() const override;
				
			private:
				std::vector<int> indices;
			};
		};
		
		std::vector<std::shared_ptr<DescriptorSet>> descriptorSets;
		
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		std::vector<VkDescriptorSet> descriptorSetsFlying;
		VkDescriptorPool descriptorPool;
		
		std::vector<VkPushConstantRange> pushConstantRanges;
		
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
		void BindDescriptorSets(int first, int number, const std::vector<int> &dynamicOffsetNumbers=std::vector<int>()) override;
		
	private:
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
	std::vector<std::shared_ptr<GraphicsPipeline>> graphicsPipelines;
	
	class ComputePipeline : public Pipeline {
	public:
		ComputePipeline(Interface &_vulkan, const ComputePipelineBlueprint &blueprint);
		~ComputePipeline(){
			vkDestroyShaderModule(vulkan.devices.logicalDevice, shaderModule, nullptr);
		}
		
		void Bind() override;
		void BindDescriptorSets(int first, int number, const std::vector<int> &dynamicOffsetNumbers=std::vector<int>()) override;
		
	private:
		VkShaderModule shaderModule;
	};
	std::vector<std::shared_ptr<ComputePipeline>> computePipelines;
	
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
	
	void CreateImage(const VkImageCreateInfo &imageCI, VkMemoryPropertyFlags properties, VkImage &image, VmaAllocation &allocation);
	VkImageView CreateImageView(const VkImageViewCreateInfo &imageViewCI);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);
	void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	
	void CreateSwapChain(const VkExtent2D &actualExtent);
	void CreateImageViews();
	void CreateColourResources();
	void CreateDepthResources();
	void CreateFramebuffers();
	void CleanUpSwapChain();
	void RecreateResisingBRPsAndImages();
	void RecreateSwapChain(){
		VkExtent2D extent = devices.GetSurfaceExtent();
		// in case we are minimised:
		while(extent.width == 0 || extent.height == 0){
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			extent = devices.GetSurfaceExtent();
		}
		
		vkDeviceWaitIdle(devices.logicalDevice);
		
		CleanUpSwapChain();
		
		CreateSwapChain(extent);
		CreateImageViews();
#ifdef MSAA
		CreateColourResources();
#endif
		CreateDepthResources();
		CreateFramebuffers();
		
		RecreateResisingBRPsAndImages();
	}
	
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VmaAllocation &allocation, VmaAllocationInfo *allocationInfoDst=nullptr);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	
	void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
};


} // namespace::EVK


#endif /* EVK_hpp */
