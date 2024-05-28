#pragma once

#include <optional>
#include <memory>
#include <cstddef>

#include "Devices.hpp"

namespace EVK {

class VertexBufferObject {
public:
	VertexBufferObject(std::shared_ptr<Devices> _devices) : devices(std::move(_devices)) {}
	~VertexBufferObject(){
		CleanUpContents();
	}
	
	bool Filled() const { return contents.has_value(); }
	
	void Fill(void *vertices, const VkDeviceSize &size, const VkDeviceSize &offset=0);
	
	bool CmdBind(VkCommandBuffer commandBuffer, uint32_t binding){
		if(!contents){
			std::cout << "Cannot bind vertex buffer, it is empty.\n";
			return false;
		}
		vkCmdBindVertexBuffers(commandBuffer, binding, 1, &contents->bufferHandle, &contents->offset);
		return true;
	}
	
private:
	std::shared_ptr<Devices> devices;
	
	struct Contents {
		VkBuffer bufferHandle;
		VkDeviceSize offset;
		VmaAllocation allocation;
		VkDeviceSize size;
	};
	std::optional<Contents> contents {};
	
	void CleanUpContents();
};

class IndexBufferObject {
public:
	IndexBufferObject(std::shared_ptr<Devices> _devices) : devices(std::move(_devices)) {}
	~IndexBufferObject(){
		CleanUpContents();
	}
	
	bool Filled() const { return contents.has_value(); }
	
	void Fill(uint32_t *indices, size_t indexCount, const VkDeviceSize &offset=0);
	
	bool CmdBind(VkCommandBuffer commandBuffer, VkIndexType type){
		if(!contents){
			std::cout << "Cannot bind index buffer, it is empty.\n";
			return false;
		}
		vkCmdBindIndexBuffer(commandBuffer, contents->bufferHandle, contents->offset, type);
		return true;
	}
	
	std::optional<uint32_t> GetIndexCount() const {
		return contents ? contents->indexCount : std::optional<uint32_t>();
		
	}
	
private:
	std::shared_ptr<Devices> devices;
	
	struct Contents {
		VkBuffer bufferHandle;
		VkDeviceSize offset;
		VmaAllocation allocation;
		uint32_t indexCount;
	};
	std::optional<Contents> contents {};
	
	void CleanUpContents();
};


struct DynamicUBOInfo {
	uint32_t repeatsN;
	VkDeviceSize alignment;
};

template <typename T, bool dynamic=false>
class UniformBufferObject {
public:
	UniformBufferObject(std::shared_ptr<Devices> _devices, uint32_t dynamicRepeats=1)
	: devices(std::move(_devices)) {
		if(dynamicRepeats < 1){
			throw std::runtime_error("UBO dynamic repeats cannot be zero.");
		}
		if constexpr (dynamic){
			// Calculate required alignment based on minimum device offset alignment
			const VkDeviceSize minUboAlignment = devices->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
			
			dynamicInfo = DynamicUBOInfo{
				.repeatsN = dynamicRepeats,
				.alignment = minUboAlignment > 0 ? (sizeof(T) + minUboAlignment - 1) & ~(minUboAlignment - 1) : sizeof(T)
			};
			size = dynamicInfo->alignment * dynamicInfo->repeatsN;
		} else { // not dynamic
			dynamicInfo.reset();
			size = sizeof(T);
		}
		for(size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i){
			devices->CreateBuffer(size,
								  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
								  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
								  buffersFlying[i],
								  allocationsFlying[i],
								  &(allocationInfosFlying[i]));
		}
	}
	~UniformBufferObject(){
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
			vmaDestroyBuffer(devices->GetAllocator(), buffersFlying[i], allocationsFlying[i]);
		}
	}
	
	// ----- Modifying UBO data -----
	T *GetDataPointer(uint32_t flight) const {
		return static_cast<T *>(allocationInfosFlying[flight].pMappedData);
	}
	std::vector<T *> GetDataPointers(uint32_t flight) const {
		std::vector<T *> ret {};
		if constexpr (dynamic){
			ret.resize(dynamicInfo->repeatsN);
			uint8_t *start = static_cast<uint8_t *>(allocationInfosFlying[flight].pMappedData);
			for(T* &ptr : ret){
				ptr = (T *)start;
				start += dynamicInfo->alignment;
			}
		} else {
			ret.push_back(static_cast<T *>(allocationInfosFlying[flight].pMappedData));
		}
		return ret;
	}
	
	VkBuffer BufferFlying(uint32_t flight) const { return buffersFlying[flight]; }
	VkDeviceSize Size() const { return size; }
	const std::optional<DynamicUBOInfo> &GetDynamic() const { return dynamicInfo; }
	
private:
	std::shared_ptr<Devices> devices;
	
	VkBuffer buffersFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation allocationsFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo allocationInfosFlying[MAX_FRAMES_IN_FLIGHT];
	VkDeviceSize size;
	
	std::optional<DynamicUBOInfo> dynamicInfo;
};

class StorageBufferObject {
public:
	StorageBufferObject(std::shared_ptr<Devices> _devices,
						VkDeviceSize _size,
						VkBufferUsageFlags usages,
						VkMemoryPropertyFlags memoryProperties);
	~StorageBufferObject(){
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
			vmaDestroyBuffer(devices->GetAllocator(), buffersFlying[i], allocationsFlying[i]);
		}
	}
	
	bool Fill(const std::vector<std::byte> &data);
	
	void CmdBindAsVertexBuffer(VkCommandBuffer commandBuffer, uint32_t binding, const VkDeviceSize &offset, uint32_t flight){
		vkCmdBindVertexBuffers(commandBuffer, binding, 1, &buffersFlying[flight], &offset);
	}
	
	VkBuffer BufferFlying(uint32_t flight) const { return buffersFlying[flight]; }
	VkDeviceSize Size() const { return size; }
	
private:
	std::shared_ptr<Devices> devices;
	
	VkBuffer buffersFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation allocationsFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo allocationInfosFlying[MAX_FRAMES_IN_FLIGHT];
	VkDeviceSize size;
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
struct Data3DImageBlueprint {
	uint8_t *data;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t pitch;
	VkFormat format;
};
struct CubemapPNGImageBlueprint {
	std::array<std::string, 6> imageFilenames;
};
struct ManualImageBlueprint {
	VkImageCreateInfo imageCI;
	VkImageViewType imageViewType;
	VkMemoryPropertyFlags properties;
	VkImageAspectFlags aspectFlags;
};

class TextureImage {
public:
	TextureImage(std::shared_ptr<Devices> _devices, PNGImageBlueprint fromPNG);
	TextureImage(std::shared_ptr<Devices> _devices, DataImageBlueprint fromRaw);
	TextureImage(std::shared_ptr<Devices> _devices, Data3DImageBlueprint fromRaw3D);
	TextureImage(std::shared_ptr<Devices> _devices, CubemapPNGImageBlueprint fromPNGCubemaps);
	TextureImage(std::shared_ptr<Devices> _devices, ManualImageBlueprint manual);
	~TextureImage(){
		vkDestroyImageView(devices->GetLogicalDevice(), view, nullptr);
		vmaDestroyImage(devices->GetAllocator(), image, allocation);
	}
	
	void CmdPipelineMemoryBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkImageSubresourceRange subresourceRange){
	 
		 const VkImageMemoryBarrier imageMemoryBarrier = {
			 .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			 .oldLayout = oldLayout,
			 .newLayout = newLayout,
			 .image = image,
			 .subresourceRange = subresourceRange,
			 .srcAccessMask = srcAccessMask,
			 .dstAccessMask = dstAccessMask,
			 .srcQueueFamilyIndex = srcQueueFamilyIndex,
			 .dstQueueFamilyIndex = dstQueueFamilyIndex
		 };
		 vkCmdPipelineBarrier(commandBuffer,
							  srcStageMask, dstStageMask, dependencyFlags,
							  0, nullptr, 0, nullptr,
							  1, &imageMemoryBarrier);
	}
	
	VkImageView View() const { return view; }
	const VkExtent3D &Extent() const { return extent; }
	VkFormat Format() const { return format; }
	VkImage Image() const { return image; }
	
private:
	std::shared_ptr<Devices> devices;
	
	VkImage image;
	VmaAllocation allocation;
	VkImageView view;
	uint32_t mipLevels;
	VkExtent3D extent;
	VkFormat format;
	
	void ConstructFromData(DataImageBlueprint _blueprint);
	void ConstructManual(ManualImageBlueprint _blueprint);
};

class TextureSampler {
public:
	TextureSampler(std::shared_ptr<Devices> _devices,
				   const VkSamplerCreateInfo &samplerCI);
	~TextureSampler(){
		vkDestroySampler(devices->GetLogicalDevice(), handle, nullptr);
	}
	
	VkSampler Handle() const { return handle; }
	
private:
	std::shared_ptr<Devices> devices;
	
	VkSampler handle;
};

class BufferedRenderPass {
public:
	BufferedRenderPass(std::shared_ptr<Devices> _devices,
					   const VkRenderPassCreateInfo *const pRenderPassCI);
	~BufferedRenderPass(){
		CleanUpTargets();
		vkDestroyRenderPass(devices->GetLogicalDevice(), renderPass, nullptr);
	}
	
	bool CmdBegin(VkCommandBuffer commandBuffer, uint32_t flight, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues){
		if(!targets){
			std::cout << "Cannot begin buffered render pass; no targets.\n";
			return false;
		}
		const VkRenderPassBeginInfo renderPassBeginInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = renderPass,
			.framebuffer = targets->frameBuffersFlying[flight],
			.renderArea = {{0, 0}, {targets->images[0]->Extent().width, targets->images[0]->Extent().height}},
			.clearValueCount = uint32_t(clearValues.size()),
			.pClearValues = clearValues.data()
		};
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, subpassContents);
		
		const VkViewport viewport{
			.x = (float)renderPassBeginInfo.renderArea.offset.x,
			.y = (float)renderPassBeginInfo.renderArea.offset.y,
			.width = (float)renderPassBeginInfo.renderArea.extent.width,
			.height = (float)renderPassBeginInfo.renderArea.extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		
		// can filter at rasterizer stage to change rendered rectangle within viewport
		const VkRect2D scissor = renderPassBeginInfo.renderArea;
		
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		return true;
	}
	
	bool Valid() const { return targets.has_value(); }
	VkRenderPass RenderPassHandle() const { return renderPass; }
	
	bool SetImages(std::vector<std::shared_ptr<TextureImage>> images);
	
private:
	std::shared_ptr<Devices> devices;
	
	VkRenderPass renderPass;
	struct Targets {
		std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> frameBuffersFlying {};
		std::vector<std::shared_ptr<TextureImage>> images {};
	};
	std::optional<Targets> targets{};
	
	void CleanUpTargets(){
		if(!targets){
			return;
		}
		for(VkFramebuffer &fb : targets->frameBuffersFlying){
			vkDestroyFramebuffer(devices->GetLogicalDevice(), fb, nullptr);
		}
		targets->images.clear();
		targets.reset();
	}
};

template <uint32_t layersN>
class LayeredBufferedRenderPass {
public:
	LayeredBufferedRenderPass(std::shared_ptr<Devices> _devices,
							  const VkRenderPassCreateInfo *const pRenderPassCI,
							  VkImageAspectFlags _imageAspectFlags)
		: devices(std::move(_devices))
		, imageAspectFlags(std::move(_imageAspectFlags)) {
			
		if(vkCreateRenderPass(devices->GetLogicalDevice(), pRenderPassCI, nullptr, &renderPass) != VK_SUCCESS){
			throw std::runtime_error("Failed to create render pass!");
		}
	}
	~LayeredBufferedRenderPass(){
		CleanUpTargets();
		vkDestroyRenderPass(devices->GetLogicalDevice(), renderPass, nullptr);
	}
	
	bool CmdBegin(VkCommandBuffer commandBuffer, uint32_t flight, const VkSubpassContents &subpassContents, const std::vector<VkClearValue> &clearValues, int layer){
		if(!targets){
			std::cout << "Cannot begin buffered render pass; no targets.\n";
			return false;
		}
		const VkRenderPassBeginInfo renderPassBeginInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = renderPass,
			.framebuffer = targets->layers[layer].frameBuffersFlying[flight],
			.renderArea = {{0, 0}, {targets->image->Extent().width, targets->image->Extent().height}},
			.clearValueCount = uint32_t(clearValues.size()),
			.pClearValues = clearValues.data()
		};
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, subpassContents);
		
		const VkViewport viewport{
			.x = (float)renderPassBeginInfo.renderArea.offset.x,
			.y = (float)renderPassBeginInfo.renderArea.offset.y,
			.width = (float)renderPassBeginInfo.renderArea.extent.width,
			.height = (float)renderPassBeginInfo.renderArea.extent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		
		// can filter at rasterizer stage to change rendered rectangle within viewport
		const VkRect2D scissor = renderPassBeginInfo.renderArea;
		
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		
		return true;
	}
	
	bool Valid() const { return targets.has_value(); }
	VkRenderPass RenderPassHandle() const { return renderPass; }
	
	void SetImage(std::shared_ptr<TextureImage> image){
		// image must be 2D??
		 
		 CleanUpTargets();
		 
		 targets = Targets();
		 targets->image = std::move(image);
		 
		 for(uint32_t i=0; i<layersN; ++i){
			 // Image view for this cascade's layer (inside the depth map)
			 // This view is used to render to that specific depth image layer
			 const VkImageViewCreateInfo imageViewCI = {
				 .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
				 .format = targets->image->Format(),
				 .subresourceRange.aspectMask = imageAspectFlags,
				 .subresourceRange.baseMipLevel = 0,
				 .subresourceRange.levelCount = 1,
				 .subresourceRange.baseArrayLayer = i,
				 .subresourceRange.layerCount = 1,
				 .image = targets->image->Image()
			 };
			 if(vkCreateImageView(devices->GetLogicalDevice(), &imageViewCI, nullptr, &(targets->layers[i].imageView)) != VK_SUCCESS){
				 throw std::runtime_error("Failed to create image view!");
			 }
			 
			 const VkFramebufferCreateInfo frameBufferCI = {
				 .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				 .renderPass = renderPass,
				 .attachmentCount = 1,
				 .pAttachments = &(targets->layers[i].imageView),
				 .width = targets->image->Extent().width,
				 .height = targets->image->Extent().height,
				 .layers = 1
			 };
			 for(VkFramebuffer &fb : targets->layers[i].frameBuffersFlying){
				 if(vkCreateFramebuffer(devices->GetLogicalDevice(), &frameBufferCI, nullptr, &fb) != VK_SUCCESS){
					 throw std::runtime_error("failed to create framebuffer!");
				 }
			 }
		 }
	 };
	
private:
	std::shared_ptr<Devices> devices;
	
	VkRenderPass renderPass;
	VkFormat imageFormat;
	VkImageAspectFlags imageAspectFlags;
	
	struct Targets {
		struct Layer {
			VkImageView imageView;
			std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> frameBuffersFlying {};
		};
		std::array<Layer, layersN> layers {};
		std::shared_ptr<TextureImage> image {};
	};
	std::optional<Targets> targets {};
	
	void CleanUpTargets(){
		if(!targets){
			return;
		}
		for(int i=0; i<layersN; ++i){
			vkDestroyImageView(devices->GetLogicalDevice(), targets->layers[i].imageView, nullptr);
			for(VkFramebuffer &fb : targets->layers[i].frameBuffersFlying){
				vkDestroyFramebuffer(devices->GetLogicalDevice(), fb, nullptr);
			}
		}
		targets->image.reset();
		targets.reset();
	}
};

} // namespace EVK
