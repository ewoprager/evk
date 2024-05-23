#include <StaticTest.hpp>

namespace Static {

VkDescriptorSetLayoutBinding UBODescriptor::LayoutBinding() const {
	return (VkDescriptorSetLayoutBinding){
		.binding = binding,
		.descriptorType = dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}
VkWriteDescriptorSet UBODescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
	bufferInfoBuffer[bufferInfoBufferIndex].buffer = buffersFlying[flight];
	bufferInfoBuffer[bufferInfoBufferIndex].offset = 0;
	bufferInfoBuffer[bufferInfoBufferIndex].range = dynamic ? dynamic->alignment : size;
	
	return (VkWriteDescriptorSet){
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dstSet,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorType = dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.pBufferInfo = &bufferInfoBuffer[bufferInfoBufferIndex++]
	};
}
VkDescriptorPoolSize UBODescriptor::PoolSize() const {
	return (VkDescriptorPoolSize){
		.type = dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1
	};
}

VkDescriptorSetLayoutBinding SBODescriptor::LayoutBinding() const {
	return (VkDescriptorSetLayoutBinding){
		.binding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}
VkWriteDescriptorSet SBODescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
	
	bufferInfoBuffer[bufferInfoBufferIndex].buffer = ref.buffersFlying[PositiveModulo(flight + flightOffset, MAX_FRAMES_IN_FLIGHT)];
	bufferInfoBuffer[bufferInfoBufferIndex].offset = 0;
	bufferInfoBuffer[bufferInfoBufferIndex].range = ref.size;
	
	return (VkWriteDescriptorSet){
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dstSet,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.pBufferInfo = &bufferInfoBuffer[bufferInfoBufferIndex++]
	};
}
VkDescriptorPoolSize SBODescriptor::PoolSize() const {
	return (VkDescriptorPoolSize){
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1
	};
}

VkDescriptorSetLayoutBinding TextureImagesDescriptor::LayoutBinding() const {
	return (VkDescriptorSetLayoutBinding){
		.binding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = uint32_t(indices.size()),
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}
VkWriteDescriptorSet TextureImagesDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
	const int startIndex = imageInfoBufferIndex;
	uint32_t n = indices.size();
	for(int k=0; k<indices.size(); k++){
		if(!descriptorSet.pipeline.vulkan.textureImages[indices[k]]){
//			throw std::runtime_error("Cannot update texture image descriptor as not all its images have been created.");
			--n;
			continue;
		}
		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoBuffer[imageInfoBufferIndex].imageView = descriptorSet.pipeline.vulkan.textureImages[indices[k]]->view;
		imageInfoBuffer[imageInfoBufferIndex].sampler = nullptr;
		imageInfoBufferIndex++;
	}
	return (VkWriteDescriptorSet){
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dstSet,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = n,//uint32_t(indices.size()),
		.pImageInfo = &imageInfoBuffer[startIndex]
	};
}
VkDescriptorPoolSize TextureImagesDescriptor::PoolSize() const {
	return (VkDescriptorPoolSize){
		.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = uint32_t(indices.size())
	};
}

VkDescriptorSetLayoutBinding TextureSamplersDescriptor::LayoutBinding() const {
	return (VkDescriptorSetLayoutBinding){
		.binding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = uint32_t(indices.size()),
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}
VkWriteDescriptorSet TextureSamplersDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
	const int startIndex = imageInfoBufferIndex;
	for(int k=0; k<indices.size(); k++){
		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoBuffer[imageInfoBufferIndex].imageView = nullptr;
		imageInfoBuffer[imageInfoBufferIndex].sampler = descriptorSet.pipeline.vulkan.textureSamplers[indices[k]];
		imageInfoBufferIndex++;
	}
	return (VkWriteDescriptorSet){
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dstSet,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = uint32_t(indices.size()),
		.pImageInfo = &imageInfoBuffer[startIndex]
	};
}
VkDescriptorPoolSize TextureSamplersDescriptor::PoolSize() const {
	return (VkDescriptorPoolSize){
		.type = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = uint32_t(indices.size())
	};
}

VkDescriptorSetLayoutBinding CombinedImageSamplersDescriptor::LayoutBinding() const {
	return (VkDescriptorSetLayoutBinding){
		.binding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = uint32_t(textureImageIndices.size()),
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}
VkWriteDescriptorSet CombinedImageSamplersDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
	const int startIndex = imageInfoBufferIndex;
	uint32_t n = textureImageIndices.size();
	for(int k=0; k<textureImageIndices.size(); k++){
		if(!descriptorSet.pipeline.vulkan.textureImages[textureImageIndices[k]]){
			//			throw std::runtime_error("Cannot update texture image descriptor as not all its images have been created.");
			--n;
			continue;
		}
		
		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoBuffer[imageInfoBufferIndex].imageView = descriptorSet.pipeline.vulkan.textureImages[textureImageIndices[k]]->view;
		imageInfoBuffer[imageInfoBufferIndex].sampler = descriptorSet.pipeline.vulkan.textureSamplers[samplerIndices[k]];
		imageInfoBufferIndex++;
	}
	return (VkWriteDescriptorSet){
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dstSet,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = n,//uint32_t(textureImageIndices.size()),
		.pImageInfo = &imageInfoBuffer[startIndex]
	};
}
VkDescriptorPoolSize CombinedImageSamplersDescriptor::PoolSize() const {
	return (VkDescriptorPoolSize){
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = uint32_t(textureImageIndices.size())
	};
}

VkDescriptorSetLayoutBinding StorageImagesDescriptor::LayoutBinding() const {
	return (VkDescriptorSetLayoutBinding){
		.binding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = uint32_t(indices.size()),
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}
VkWriteDescriptorSet StorageImagesDescriptor::DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const {
	const int startIndex = imageInfoBufferIndex;
	uint32_t n = indices.size();
	for(int k=0; k<indices.size(); k++){
		if(!descriptorSet.pipeline.vulkan.textureImages[indices[k]]){
			//			throw std::runtime_error("Cannot update storage image descriptor as not all its images have been created.");
			--n;
			continue;
		}
		
		imageInfoBuffer[imageInfoBufferIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL; // or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR?; https://registry.khronos.org/vulkan/site/spec/latest/chapters/descriptorsets.html
		imageInfoBuffer[imageInfoBufferIndex].imageView = descriptorSet.pipeline.vulkan.textureImages[indices[k]]->view;
		imageInfoBuffer[imageInfoBufferIndex].sampler = nullptr;
		imageInfoBufferIndex++;
	}
	
	return (VkWriteDescriptorSet){
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dstSet,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = n,//uint32_t(indices.size()),
		.pImageInfo = &imageInfoBuffer[startIndex]
	};
}
VkDescriptorPoolSize StorageImagesDescriptor::PoolSize() const {
	return (VkDescriptorPoolSize){
		.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = uint32_t(indices.size())
	};
}


} // namespace Static
