#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags>
class SBODescriptor : public DescriptorBase<binding, stageFlags> {
public:
	SBODescriptor(int _index, int _flightOffset) : index(_index), flightOffset(_flightOffset) {}
	
	VkDescriptorSetLayoutBinding LayoutBinding() const override;
	
	VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
	
	VkDescriptorPoolSize PoolSize() const override;
	
private:
	int index;
	int flightOffset;
};

} // namespace EVK

