#pragma once

#include <Header.hpp>

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags> struct DescriptorBase {
	static constexpr uint32_t bindingValue = binding;
	static constexpr VkShaderStageFlags stageFlagsValue = stageFlags;
	
	virtual VkDescriptorSetLayoutBinding LayoutBinding() const = 0;
	
	virtual VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const = 0;
	
	virtual VkDescriptorPoolSize PoolSize() const = 0;
};

} // namespace EVK
