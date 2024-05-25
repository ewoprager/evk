#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags, uint32_t samplerCount>
class TextureSamplersDescriptor : public DescriptorBase<binding, stageFlags> {
public:
	TextureSamplersDescriptor() {}
	
	static consteval VkDescriptorSetLayoutBinding LayoutBinding() const override {
		return (VkDescriptorSetLayoutBinding){
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = samplerCount,
			.stageFlags = stageFlags,
			.pImmutableSamplers = nullptr
		};
	}
	
	std::optional<VkWriteDescriptorSet> DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override {
		for(std::shared_ptr<TextureSampler> sampler : samplers){
			if(!sampler){
				return {};
			}
		}
		
		const int startIndex = imageInfoBufferIndex;
		for(std::shared_ptr<TextureSampler> sampler : samplers){
			imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfoBuffer[imageInfoBufferIndex].imageView = nullptr;
			imageInfoBuffer[imageInfoBufferIndex].sampler = sampler->Handle();
			imageInfoBufferIndex++;
		}
		return (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = dstSet,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = samplerCount,
			.pImageInfo = &imageInfoBuffer[startIndex]
		};
	}
	
	static consteval VkDescriptorPoolSize PoolSize() const override {
		return (VkDescriptorPoolSize){
			.type = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = samplerCount
		};
	}
	
private:
	std::array<std::shared_ptr<TextureSampler>, samplerCount> samplers {};
};

} // namespace EVK

