#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags, bool dynamic>
class UBODescriptor : public DescriptorBase<binding, stageFlags> {
public:
	UBODescriptor() = default;
	
	void Set(std::shared_ptr<UniformBufferObject> value){
		object = std::move(value);
		DescriptorBase<binding, stageFlags>::valid = false;
	}
	
	static constexpr VkDescriptorSetLayoutBinding layoutBinding  = {
		.binding = binding,
		.descriptorType = dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
	
	std::optional<VkWriteDescriptorSet> DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override {
		if(!object){
			return {};
		}
		
		bufferInfoBuffer[bufferInfoBufferIndex].buffer = object->BufferFlying(flight);
		bufferInfoBuffer[bufferInfoBufferIndex].offset = 0;
		bufferInfoBuffer[bufferInfoBufferIndex].range = dynamic ? object->GetDynamic()->alignment : object->Size();
		
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
	
	static constexpr VkDescriptorPoolSize poolSize = {
		.type = dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1 * MAX_FRAMES_IN_FLIGHT
	};
	
	std::optional<UniformBufferObject::Dynamic> GetUBODynamic() const override {
		if(!object){
			std::cout << "Cannot get if UBO is dynamic; no object.\n";
			return {};
		}
		return object->GetDynamic();
	}
	
private:
	std::shared_ptr<UniformBufferObject> object {};
};

} // namespace EVK
