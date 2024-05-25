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
	
	void Fill(void *vertices, const VkDeviceSize &size, const VkDeviceSize &offset);
	
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
	
	void Fill(uint32_t *indices, size_t indexCount, const VkDeviceSize &offset);
	
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

class UniformBufferObject {
public:
	UniformBufferObject(std::shared_ptr<Devices> _devices, VkDeviceSize size);
	~UniformBufferObject(){
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
			vmaDestroyBuffer(devices->GetAllocator(), buffersFlying[i], allocationsFlying[i]);
		}
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
	VkImageCreateInfo imageCI; // is this safe? given that we pass these around
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
	
	VkImageView View() const { return view; }
	const VkExtent3D &Extent() const { return blueprint.imageCI.extent; }
	VkFormat Format() const { return blueprint.imageCI.format; }
	VkImage Image() const { return image; }
	
private:
	std::shared_ptr<Devices> devices;
	
	VkImage image;
	VmaAllocation allocation;
	VkImageView view;
	uint32_t mipLevels;
	ManualImageBlueprint blueprint;
	
	void ConstructFromData(DataImageBlueprint _blueprint);
	void ConstructManual(ManualImageBlueprint _blueprint);
};

class TextureSampler {
public:
	TextureSampler(std::shared_ptr<Devices> _devices,
				   const VkSamplerCreateInfo &samplerCI);
	~TextureSampler(){
		vkDestroySampler(device->GetLogicalDevice(), handle, nullptr);
	}
	
	VkSampler Handle() const { return handle; }
	
private:
	std::shared_ptr<Devices> devices;
	
	VkSampler handle;
};

class BufferedRenderPass {
public:
	BufferedRenderPass(std::shared_ptr<Devices> _devices,
					   const VkRenderPassCreateInfo &renderPassCI);
	~BufferedRenderPass(){
		CleanUpTargets();
		vkDestroyRenderPass(devices->GetLogicalDevice(), renderPass, nullptr);
	}
	
	bool Valid() const { return targets.has_value(); }
	
	void SetImages(std::vector<std::shared_ptr<TextureImage>> images);
	
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
		target.reset();
	}
};

template <uint32_t layersN>
class LayeredBufferedRenderPass {
public:
	LayeredBufferedRenderPass(std::shared_ptr<Devices> _devices,
							  const VkRenderPassCreateInfo &renderPassCI,
							  const VkImageAspectFlags &_imageAspectFlags)
		: devices(std::move(_devices))
		, imageAspectFlags(_imageAspectFlags) {
			
		if(vkCreateRenderPass(devices->GetLogicalDevice(), &renderPassCI, nullptr, &renderPass) != VK_SUCCESS){
			throw std::runtime_error("Failed to create render pass!");
		}
	}
	~LayeredBufferedRenderPass(){
		CleanUpTargets();
		vkDestroyRenderPass(devices->GetLogicalDevice(), renderPass, nullptr);
	}
	
	bool Valid() const { return targets.has_value(); }
	
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
			if(vkCreateImageView(devices.GetLogicalDevice(), &imageViewCI, nullptr, &(targets->layers[i].imageView) != VK_SUCCESS){
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
				if(vkCreateFramebuffer(devices.GetLogicalDevice(), &frameBufferCI, nullptr, &fb) != VK_SUCCESS){
					throw std::runtime_error("failed to create framebuffer!");
				}
			}
		}
	}
	
private:
	std::shared_ptr<Devices> devices;
	
	uint32_t width, height;
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
		for(Targets::Layer &layer : targets->layers){
			vkDestroyImageView(devices->GetLogicalDevice(), layer.imageView, nullptr);
			for(VkFramebuffer &fb : layer.frameBuffersFlying){
				vkDestroyFramebuffer(devices->GetLogicalDevice(), fb, nullptr);
			}
		}
		targets->image.reset();
		targets.reset();
	}
};

} // namespace EVK
