#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags, uint32_t imageCount>
class TextureImagesDescriptor : public DescriptorBase<binding, stageFlags> {
public:
	TextureImagesDescriptor() = default;
	
	void Set(std::array<std::shared_ptr<TextureImage>, imageCount> value){
		images = std::move(value);
		DescriptorBase<binding, stageFlags>::valid = false;
	}
	
	static constexpr VkDescriptorSetLayoutBinding layoutBinding = {
		.binding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = imageCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
	
	std::optional<VkWriteDescriptorSet> DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override {
		for(std::shared_ptr<TextureImage> image : images){
			if(!image){
				return {};
			}
		}
		const int startIndex = imageInfoBufferIndex;
		for(std::shared_ptr<TextureImage> image : images){
			imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfoBuffer[imageInfoBufferIndex].imageView = image->View();
			imageInfoBuffer[imageInfoBufferIndex].sampler = nullptr;
			imageInfoBufferIndex++;
		}
		
		return (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = dstSet,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = imageCount,
			.pImageInfo = &imageInfoBuffer[startIndex]
		};
	}
	
	static constexpr VkDescriptorPoolSize poolSize = {
		.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = imageCount * MAX_FRAMES_IN_FLIGHT
	};
	
private:
	std::array<std::shared_ptr<TextureImage>, imageCount> images {};
};

} // namespace EVK
