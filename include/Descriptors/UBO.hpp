#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags>
class UBODescriptor : public DescriptorBase<binding, stageFlags> {
public:
	UBODescriptor(int _index) : index(_index) {}
	
	VkDescriptorSetLayoutBinding LayoutBinding() const override;
	
	VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
	
	VkDescriptorPoolSize PoolSize() const override;
	
private:
	int index;
};

} // namespace EVK
