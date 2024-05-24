#pragma once

#include "Devices.hpp"

namespace EVK {

class Validated {
public:
	bool Valid() const { return valid; }
	
protected:
	bool valid = false;
}

class VertexBufferObject : public Validated {
public:
	VertexBufferObject(const VmaAllocator &_allocator) : allocator(_allocator) {}
	~VertexBufferObject(){
		if(valid){
			vmaDestroyBuffer(allocator, bufferHandle, allocation);
		}
	}
	
private:
	const VmaAllocator &allocator;
		
	VkBuffer bufferHandle;
	VkDeviceSize offset;
	VmaAllocation allocation;
	VkDeviceSize size;
};

class IndexBufferObject : public Validated {
public:
	IndexBufferObject(const VmaAllocator &_allocator) : allocator(_allocator) {}
	~IndexBufferObject(){
		if(valid){
			vmaDestroyBuffer(allocator, bufferHandle, allocation);
		}
	}
	
private:
	const VmaAllocator &allocator;
	
	VkBuffer bufferHandle;
	VkDeviceSize offset;
	VmaAllocation allocation;
	uint32_t indexCount;
};

class UniformBufferObject : public Validated {
public:
	UniformBufferObject(const VmaAllocator &_allocator) : allocator(_allocator) {}
	~UniformBufferObject(){
		if(valid){
			for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
				vmaDestroyBuffer(allocator, buffersFlying[i], allocationsFlying[i]);
			}
		}
	}
	
private:
	const VmaAllocator &allocator;
	
	VkBuffer buffersFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation allocationsFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo allocationInfosFlying[MAX_FRAMES_IN_FLIGHT];
	VkDeviceSize size;
};

class StorageBufferObject : public Validated {
public:
	StorageBufferObject(const VmaAllocator &_allocator) : allocator(_allocator) {}
	~StorageBufferObject(){
		if(valid){
			for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
				vmaDestroyBuffer(allocator, buffersFlying[i], allocationsFlying[i]);
			}
		}
	}
	
private:
	const VmaAllocator &allocator;
	
	VkBuffer buffersFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocation allocationsFlying[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo allocationInfosFlying[MAX_FRAMES_IN_FLIGHT];
	VkDeviceSize size;
};

class TextureImage : public Validated {
public:
	TextureImage(const VkDevice &_logicalDevice, const VmaAllocator &_allocator) : logicalDevice(_logicalDevice), allocator(_allocator) {}
	~TextureImage(){
		if(valid){
			vkDestroyImageView(logicalDevice, view, nullptr);
			vmaDestroyImage(allocator, image, allocation);
		}
	}
	
private:
	const VkDevice &logicalDevice;
	const VmaAllocator &allocator;
	
	VkImage image;
	VmaAllocation allocation;
	VkImageView view;
	uint32_t mipLevels;
	ManualImageBlueprint blueprint;
};

class BufferedRenderPass : public Validated {
public:
	BufferedRenderPass(const VkDevice &_logicalDevice) : logicalDevice(_logicalDevice) {}
	~BufferedRenderPass(){
		if(valid){
			vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
			for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
				vkDestroyFramebuffer(logicalDevice, frameBuffersFlying[i], nullptr);
			}
		}
	}
	
private:
	const VkDevice &logicalDevice;
	
	VkRenderPass renderPass;
	std::vector<int> targetTextureImageIndices;
	VkFramebuffer frameBuffersFlying[MAX_FRAMES_IN_FLIGHT];
	std::optional<vec<2, uint32_t>> size;
};

class LayeredBufferedRenderPass : public Validated {
public:
	LayeredBufferedRenderPass(const VkDevice &_logicalDevice) : logicalDevice(_logicalDevice) {}
	~LayeredBufferedRenderPass(){
		if(valid){
			vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
			for(Layer &layer : layers){
				layer.CleanUp(logicalDevice);
			}
		}
	}
	
private:
	const VkDevice &logicalDevice;
//	
//	LayeredBufferedRenderPass(int layersN) {
//		layers = std::vector<Layer>(layersN);
//	}
	
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
	std::vector<Layer> layers {};
};

} // namespace EVK
