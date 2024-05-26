#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags, uint32_t count>
class CombinedImageSamplersDescriptor : public DescriptorBase<binding, stageFlags> {
public:
	CombinedImageSamplersDescriptor() = default;
	
	struct Combo {
		std::shared_ptr<TextureImage> image;
		std::shared_ptr<TextureSampler> sampler;
	};
	
	void Set(std::array<Combo, count> value){
		combos = std::move(value);
		DescriptorBase<binding, stageFlags>::valid = false;
	}
	
	static constexpr VkDescriptorSetLayoutBinding layoutBinding = {
		.binding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = count,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
	
	std::optional<VkWriteDescriptorSet> DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override {
		for(Combo &combo : combos){
			if(!combo.image || !combo.sampler){
				return {};
			}
		}
		const int startIndex = imageInfoBufferIndex;
		for(Combo &combo : combos){
			imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfoBuffer[imageInfoBufferIndex].imageView = combo.image->View();
			imageInfoBuffer[imageInfoBufferIndex].sampler = combo.sampler->Handle();
			imageInfoBufferIndex++;
		}
		
		return (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = dstSet,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = count,
			.pImageInfo = &imageInfoBuffer[startIndex]
		};
	}
	
	static constexpr VkDescriptorPoolSize poolSize = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = count * MAX_FRAMES_IN_FLIGHT
	};
	
private:
	std::array<Combo, count> combos {};
};

} // namespace EVK

