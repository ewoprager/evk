#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags, uint32_t imageCount>
class StorageImagesDescriptor : public DescriptorBase<binding, stageFlags> {
public:
	StorageImagesDescriptor() = default;
	
	static consteval VkDescriptorSetLayoutBinding LayoutBinding() {
		return (VkDescriptorSetLayoutBinding){
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = imageCount,
			.stageFlags = stageFlags,
			.pImmutableSamplers = nullptr
		};
	}
	
	std::optional<VkWriteDescriptorSet> DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override {
		for(std::shared_ptr<TextureImage> image : images){
			if(!image){
				return {};
			}
		}
		const int startIndex = imageInfoBufferIndex;
		for(std::shared_ptr<TextureImage> image : images){
			imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL; // or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR?; https://registry.khronos.org/vulkan/site/spec/latest/chapters/descriptorsets.html
			imageInfoBuffer[imageInfoBufferIndex].imageView = image->View();
			imageInfoBuffer[imageInfoBufferIndex].sampler = nullptr;
			imageInfoBufferIndex++;
		}

		return (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = dstSet,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = imageCount,
			.pImageInfo = &imageInfoBuffer[startIndex]
		};
	}
	
	static consteval VkDescriptorPoolSize PoolSize() {
		return (VkDescriptorPoolSize){
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = imageCount * MAX_FRAMES_IN_FLIGHT
		};
	}
	
private:
	std::array<std::shared_ptr<TextureImage>, imageCount> images {};
};

} // namespace EVK
