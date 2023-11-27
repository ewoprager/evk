#include <Base.hpp>

namespace EVK {

VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::UBODescriptor::LayoutBinding() const {
	UniformBufferObject &ref = descriptorSet.pipeline.vulkan.uniformBufferObjects[index];
	VkDescriptorSetLayoutBinding ret = {};
	ret.binding = binding;
	ret.descriptorType = ref.dynamicRepeats > 1 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ret.descriptorCount = 1;
	ret.stageFlags = stageFlags;
	ret.pImmutableSamplers = nullptr;
	return ret;
}
VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::UBODescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const {
	UniformBufferObject &ref = descriptorSet.pipeline.vulkan.uniformBufferObjects[index];
	VkWriteDescriptorSet ret = {};
	ret.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ret.dstSet = dstSet;
	ret.dstBinding = binding;
	ret.dstArrayElement = 0;
	ret.descriptorType = ref.dynamicRepeats > 1 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ret.descriptorCount = 1;
	bufferInfoBuffer[bufferInfoBufferIndex].buffer = ref.buffersFlying[flight];
	bufferInfoBuffer[bufferInfoBufferIndex].offset = 0;
	bufferInfoBuffer[bufferInfoBufferIndex].range = ref.dynamicRepeats > 1 ? ref.dynamicAlignment : ref.size;
	ret.pBufferInfo = &bufferInfoBuffer[bufferInfoBufferIndex++];
	return ret;
}
VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::UBODescriptor::PoolSize() const {
	VkDescriptorPoolSize ret = {};
	UniformBufferObject &ref = descriptorSet.pipeline.vulkan.uniformBufferObjects[index];
	ret.type = ref.dynamicRepeats > 1 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ret.descriptorCount = 1;
	return ret;
}

VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::SBODescriptor::LayoutBinding() const {
	VkDescriptorSetLayoutBinding ret = {};
	ret.binding = binding;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ret.descriptorCount = 1;
	ret.stageFlags = stageFlags;
	ret.pImmutableSamplers = nullptr;
	return ret;
}
VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::SBODescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const {
	StorageBufferObject &ref = descriptorSet.pipeline.vulkan.storageBufferObjects[index];
	VkWriteDescriptorSet ret = {};
	ret.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ret.dstSet = dstSet;
	ret.dstBinding = binding;
	ret.dstArrayElement = 0;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ret.descriptorCount = 1;
	bufferInfoBuffer[bufferInfoBufferIndex].buffer = ref.buffersFlying[flight];
	bufferInfoBuffer[bufferInfoBufferIndex].offset = 0;
	bufferInfoBuffer[bufferInfoBufferIndex].range = ref.size;
	ret.pBufferInfo = &bufferInfoBuffer[bufferInfoBufferIndex++];
	return ret;
}
VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::SBODescriptor::PoolSize() const {
	VkDescriptorPoolSize ret = {};
	ret.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ret.descriptorCount = 1;
	return ret;
}

VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::TextureImagesDescriptor::LayoutBinding() const {
	VkDescriptorSetLayoutBinding ret = {};
	ret.binding = binding;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	ret.descriptorCount = count;
	ret.stageFlags = stageFlags;
	ret.pImmutableSamplers = nullptr;
	return ret;
}
VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::TextureImagesDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const {
	VkWriteDescriptorSet ret = {};
	ret.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ret.dstSet = dstSet;
	ret.dstBinding = binding;
	ret.dstArrayElement = 0;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	ret.descriptorCount = count;
	const int startIndex = imageInfoBufferIndex;
	for(int k=0; k<count; k++){
		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoBuffer[imageInfoBufferIndex].imageView = descriptorSet.pipeline.vulkan.textureImages[indices[k]].view;
		imageInfoBuffer[imageInfoBufferIndex].sampler = nullptr;
		imageInfoBufferIndex++;
	}
	ret.pImageInfo = &imageInfoBuffer[startIndex];
	return ret;
}
VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::TextureImagesDescriptor::PoolSize() const {
	VkDescriptorPoolSize ret = {};
	ret.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	ret.descriptorCount = count;
	return ret;
}

VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::TextureSamplersDescriptor::LayoutBinding() const {
	VkDescriptorSetLayoutBinding ret = {};
	ret.binding = binding;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	ret.descriptorCount = count;
	ret.stageFlags = stageFlags;
	ret.pImmutableSamplers = nullptr;
	return ret;
}
VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::TextureSamplersDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const {
	VkWriteDescriptorSet ret = {};
	ret.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ret.dstSet = dstSet;
	ret.dstBinding = binding;
	ret.dstArrayElement = 0;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	ret.descriptorCount = count;
	const int startIndex = imageInfoBufferIndex;
	for(int k=0; k<count; k++){
		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoBuffer[imageInfoBufferIndex].imageView = nullptr;
		imageInfoBuffer[imageInfoBufferIndex].sampler = descriptorSet.pipeline.vulkan.textureSamplers[indices[k]];
		imageInfoBufferIndex++;
	}
	ret.pImageInfo = &imageInfoBuffer[startIndex];
	return ret;
}
VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::TextureSamplersDescriptor::PoolSize() const {
	VkDescriptorPoolSize ret = {};
	ret.type = VK_DESCRIPTOR_TYPE_SAMPLER;
	ret.descriptorCount = count;
	return ret;
}

VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::CombinedImageSamplersDescriptor::LayoutBinding() const {
	VkDescriptorSetLayoutBinding ret = {};
	ret.binding = binding;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ret.descriptorCount = count;
	ret.stageFlags = stageFlags;
	ret.pImmutableSamplers = nullptr;
	return ret;
}
VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::CombinedImageSamplersDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const {
	VkWriteDescriptorSet ret = {};
	ret.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ret.dstSet = dstSet;
	ret.dstBinding = binding;
	ret.dstArrayElement = 0;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ret.descriptorCount = count;
	const int startIndex = imageInfoBufferIndex;
	for(int k=0; k<count; k++){
		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoBuffer[imageInfoBufferIndex].imageView = descriptorSet.pipeline.vulkan.textureImages[textureImageIndices[k]].view;
		imageInfoBuffer[imageInfoBufferIndex].sampler = descriptorSet.pipeline.vulkan.textureSamplers[samplerIndices[k]];
		imageInfoBufferIndex++;
	}
	ret.pImageInfo = &imageInfoBuffer[startIndex];
	return ret;
}
VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::CombinedImageSamplersDescriptor::PoolSize() const {
	VkDescriptorPoolSize ret = {};
	ret.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	ret.descriptorCount = count;
	return ret;
}

VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::StorageImagesDescriptor::LayoutBinding() const {
	VkDescriptorSetLayoutBinding ret = {};
	ret.binding = binding;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	ret.descriptorCount = count;
	ret.stageFlags = stageFlags;
	ret.pImmutableSamplers = nullptr;
	return ret;
}
VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::StorageImagesDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *const &imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *const &bufferInfoBuffer, int &bufferInfoBufferIndex, const int &flight) const {
	VkWriteDescriptorSet ret = {};
	ret.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ret.dstSet = dstSet;
	ret.dstBinding = binding;
	ret.dstArrayElement = 0;
	ret.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	ret.descriptorCount = count;
	const int startIndex = imageInfoBufferIndex;
	for(int k=0; k<count; k++){
		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL; // or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR?; https://registry.khronos.org/vulkan/site/spec/latest/chapters/descriptorsets.html
		imageInfoBuffer[imageInfoBufferIndex].imageView = descriptorSet.pipeline.vulkan.textureImages[indices[k]].view;
		imageInfoBuffer[imageInfoBufferIndex].sampler = nullptr;
		imageInfoBufferIndex++;
	}
	ret.pImageInfo = &imageInfoBuffer[startIndex];
	return ret;
}
VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::StorageImagesDescriptor::PoolSize() const {
	VkDescriptorPoolSize ret = {};
	ret.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	ret.descriptorCount = count;
	return ret;
}

} // namespace::EVK
