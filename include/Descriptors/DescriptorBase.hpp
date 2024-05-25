#pragma once

#include "Header.hpp"

#include "Resources.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags>
struct DescriptorBase {
	static constexpr uint32_t bindingValue = binding;
	static constexpr VkShaderStageFlags stageFlagsValue = stageFlags;
	
	static virtual consteval VkDescriptorSetLayoutBinding LayoutBinding() const = 0;
	
	virtual std::optional<VkWriteDescriptorSet> DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const = 0;
	
	static virtual consteval VkDescriptorPoolSize PoolSize() const = 0;
};

} // namespace EVK
