#include <Base.hpp>

namespace EVK {

//
//
//VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::UBODescriptor::LayoutBinding() const {
//	if(!descriptorSet.pipeline.vulkan.uniformBufferObjects[index])
//		throw std::runtime_error("Cannot get UBO layout binding as it hasn't been created.");
//	UniformBufferObject &ref = descriptorSet.pipeline.vulkan.uniformBufferObjects[index].value();
//	
//	return (VkDescriptorSetLayoutBinding){
//		.binding = binding,
//		.descriptorType = ref.dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//		.descriptorCount = 1,
//		.stageFlags = stageFlags,
//		.pImmutableSamplers = nullptr
//	};
//}
//VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::UBODescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
//	if(!descriptorSet.pipeline.vulkan.uniformBufferObjects[index])
//		throw std::runtime_error("Cannot update UBO descriptor as it hasn't been created.");
//	UniformBufferObject &ref = descriptorSet.pipeline.vulkan.uniformBufferObjects[index].value();
//	
//	bufferInfoBuffer[bufferInfoBufferIndex].buffer = ref.buffersFlying[flight];
//	bufferInfoBuffer[bufferInfoBufferIndex].offset = 0;
//	bufferInfoBuffer[bufferInfoBufferIndex].range = ref.dynamic ? ref.dynamic->alignment : ref.size;
//	
//	return (VkWriteDescriptorSet){
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = dstSet,
//		.dstBinding = binding,
//		.dstArrayElement = 0,
//		.descriptorType = ref.dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//		.descriptorCount = 1,
//		.pBufferInfo = &bufferInfoBuffer[bufferInfoBufferIndex++]
//	};
//}
//VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::UBODescriptor::PoolSize() const {
//	if(!descriptorSet.pipeline.vulkan.uniformBufferObjects[index])
//		throw std::runtime_error("Cannot get UBO pool size as it hasn't been created.");
//	UniformBufferObject &ref = descriptorSet.pipeline.vulkan.uniformBufferObjects[index].value();
//	
//	return (VkDescriptorPoolSize){
//		.type = ref.dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//		.descriptorCount = 1
//	};
//}
//
//VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::SBODescriptor::LayoutBinding() const {
//	return (VkDescriptorSetLayoutBinding){
//		.binding = binding,
//		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//		.descriptorCount = 1,
//		.stageFlags = stageFlags,
//		.pImmutableSamplers = nullptr
//	};
//}
//VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::SBODescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
//	if(!descriptorSet.pipeline.vulkan.storageBufferObjects[index])
//		throw std::runtime_error("Cannot update SBO descriptor as it hasn't been created.");
//	StorageBufferObject &ref = descriptorSet.pipeline.vulkan.storageBufferObjects[index].value();
//	
//	bufferInfoBuffer[bufferInfoBufferIndex].buffer = ref.buffersFlying[PositiveModulo(flight + flightOffset, MAX_FRAMES_IN_FLIGHT)];
//	bufferInfoBuffer[bufferInfoBufferIndex].offset = 0;
//	bufferInfoBuffer[bufferInfoBufferIndex].range = ref.size;
//	
//	return (VkWriteDescriptorSet){
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = dstSet,
//		.dstBinding = binding,
//		.dstArrayElement = 0,
//		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//		.descriptorCount = 1,
//		.pBufferInfo = &bufferInfoBuffer[bufferInfoBufferIndex++]
//	};
//}
//VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::SBODescriptor::PoolSize() const {
//	return (VkDescriptorPoolSize){
//		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//		.descriptorCount = 1
//	};
//}
//
//VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::TextureImagesDescriptor::LayoutBinding() const {
//	return (VkDescriptorSetLayoutBinding){
//		.binding = binding,
//		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
//		.descriptorCount = uint32_t(indices.size()),
//		.stageFlags = stageFlags,
//		.pImmutableSamplers = nullptr
//	};
//}
//VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::TextureImagesDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
//	const int startIndex = imageInfoBufferIndex;
//	uint32_t n = indices.size();
//	for(int k=0; k<indices.size(); k++){
//		if(!descriptorSet.pipeline.vulkan.textureImages[indices[k]]){
////			throw std::runtime_error("Cannot update texture image descriptor as not all its images have been created.");
//			--n;
//			continue;
//		}
//		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//		imageInfoBuffer[imageInfoBufferIndex].imageView = descriptorSet.pipeline.vulkan.textureImages[indices[k]]->view;
//		imageInfoBuffer[imageInfoBufferIndex].sampler = nullptr;
//		imageInfoBufferIndex++;
//	}
//	return (VkWriteDescriptorSet){
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = dstSet,
//		.dstBinding = binding,
//		.dstArrayElement = 0,
//		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
//		.descriptorCount = n,//uint32_t(indices.size()),
//		.pImageInfo = &imageInfoBuffer[startIndex]
//	};
//}
//VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::TextureImagesDescriptor::PoolSize() const {
//	return (VkDescriptorPoolSize){
//		.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
//		.descriptorCount = uint32_t(indices.size())
//	};
//}
//
//VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::TextureSamplersDescriptor::LayoutBinding() const {
//	return (VkDescriptorSetLayoutBinding){
//		.binding = binding,
//		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
//		.descriptorCount = uint32_t(indices.size()),
//		.stageFlags = stageFlags,
//		.pImmutableSamplers = nullptr
//	};
//}
//VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::TextureSamplersDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
//	const int startIndex = imageInfoBufferIndex;
//	for(int k=0; k<indices.size(); k++){
//		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//		imageInfoBuffer[imageInfoBufferIndex].imageView = nullptr;
//		imageInfoBuffer[imageInfoBufferIndex].sampler = descriptorSet.pipeline.vulkan.textureSamplers[indices[k]];
//		imageInfoBufferIndex++;
//	}
//	return (VkWriteDescriptorSet){
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = dstSet,
//		.dstBinding = binding,
//		.dstArrayElement = 0,
//		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
//		.descriptorCount = uint32_t(indices.size()),
//		.pImageInfo = &imageInfoBuffer[startIndex]
//	};
//}
//VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::TextureSamplersDescriptor::PoolSize() const {
//	return (VkDescriptorPoolSize){
//		.type = VK_DESCRIPTOR_TYPE_SAMPLER,
//		.descriptorCount = uint32_t(indices.size())
//	};
//}
//
//VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::CombinedImageSamplersDescriptor::LayoutBinding() const {
//	return (VkDescriptorSetLayoutBinding){
//		.binding = binding,
//		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//		.descriptorCount = uint32_t(textureImageIndices.size()),
//		.stageFlags = stageFlags,
//		.pImmutableSamplers = nullptr
//	};
//}
//VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::CombinedImageSamplersDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
//	const int startIndex = imageInfoBufferIndex;
//	uint32_t n = textureImageIndices.size();
//	for(int k=0; k<textureImageIndices.size(); k++){
//		if(!descriptorSet.pipeline.vulkan.textureImages[textureImageIndices[k]]){
//			//			throw std::runtime_error("Cannot update texture image descriptor as not all its images have been created.");
//			--n;
//			continue;
//		}
//		
//		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//		imageInfoBuffer[imageInfoBufferIndex].imageView = descriptorSet.pipeline.vulkan.textureImages[textureImageIndices[k]]->view;
//		imageInfoBuffer[imageInfoBufferIndex].sampler = descriptorSet.pipeline.vulkan.textureSamplers[samplerIndices[k]];
//		imageInfoBufferIndex++;
//	}
//	return (VkWriteDescriptorSet){
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = dstSet,
//		.dstBinding = binding,
//		.dstArrayElement = 0,
//		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//		.descriptorCount = n,//uint32_t(textureImageIndices.size()),
//		.pImageInfo = &imageInfoBuffer[startIndex]
//	};
//}
//VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::CombinedImageSamplersDescriptor::PoolSize() const {
//	return (VkDescriptorPoolSize){
//		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//		.descriptorCount = uint32_t(textureImageIndices.size())
//	};
//}
//
//VkDescriptorSetLayoutBinding Interface::Pipeline::DescriptorSet::StorageImagesDescriptor::LayoutBinding() const {
//	return (VkDescriptorSetLayoutBinding){
//		.binding = binding,
//		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
//		.descriptorCount = uint32_t(indices.size()),
//		.stageFlags = stageFlags,
//		.pImmutableSamplers = nullptr
//	};
//}
//VkWriteDescriptorSet Interface::Pipeline::DescriptorSet::StorageImagesDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
//	const int startIndex = imageInfoBufferIndex;
//	uint32_t n = indices.size();
//	for(int k=0; k<indices.size(); k++){
//		if(!descriptorSet.pipeline.vulkan.textureImages[indices[k]]){
//			//			throw std::runtime_error("Cannot update storage image descriptor as not all its images have been created.");
//			--n;
//			continue;
//		}
//		
//		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL; // or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR?; https://registry.khronos.org/vulkan/site/spec/latest/chapters/descriptorsets.html
//		imageInfoBuffer[imageInfoBufferIndex].imageView = descriptorSet.pipeline.vulkan.textureImages[indices[k]]->view;
//		imageInfoBuffer[imageInfoBufferIndex].sampler = nullptr;
//		imageInfoBufferIndex++;
//	}
//	
//	return (VkWriteDescriptorSet){
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = dstSet,
//		.dstBinding = binding,
//		.dstArrayElement = 0,
//		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
//		.descriptorCount = n,//uint32_t(indices.size()),
//		.pImageInfo = &imageInfoBuffer[startIndex]
//	};
//}
//VkDescriptorPoolSize Interface::Pipeline::DescriptorSet::StorageImagesDescriptor::PoolSize() const {
//	return (VkDescriptorPoolSize){
//		.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
//		.descriptorCount = uint32_t(indices.size())
//	};
//}

} // namespace::EVK
