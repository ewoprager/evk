#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags>
class CombinedImageSamplersDescriptor : public DescriptorBase<binding, stageFlags> {
public:
	CombinedImageSamplersDescriptor(const std::vector<int> &_textureImageIndices, const std::vector<int> &_samplerIndices) : textureImageIndices(_textureImageIndices), samplerIndices(_samplerIndices) {
		assert(_textureImageIndices.size() == _samplerIndices.size());
	}
	
	VkDescriptorSetLayoutBinding LayoutBinding() const override;
	
	VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
	
	VkDescriptorPoolSize PoolSize() const override;
	
private:
	std::vector<int> textureImageIndices;
	std::vector<int> samplerIndices;
};

} // namespace EVK

