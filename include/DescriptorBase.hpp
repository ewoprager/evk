#pragma once

#include "Header.hpp"

#include "Resources.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags>
class DescriptorBase {
public:
	static constexpr uint32_t bindingValue = binding;
	static constexpr VkShaderStageFlags stageFlagsValue = stageFlags;
	
	bool Valid() const { return valid; }
	void SetValid(){ valid = true; }
	
//	static virtual constexpr VkDescriptorSetLayoutBinding layoutBinding = 0;
	
	virtual std::optional<VkWriteDescriptorSet> DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const = 0;
	
//	static virtual constexpr VkDescriptorPoolSize poolSize = 0;
	
	virtual std::optional<UniformBufferObject::Dynamic> GetUBODynamic() const { return {}; }
	
protected:
	bool valid = false;
};

} // namespace EVK
