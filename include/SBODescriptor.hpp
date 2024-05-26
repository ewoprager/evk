#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags>
class SBODescriptor : public DescriptorBase<binding, stageFlags> {
public:
	using DescriptorBase = typename SBODescriptor::DescriptorBase;
	
	SBODescriptor() = default;
	
	void Set(std::shared_ptr<StorageBufferObject> value){
		object = std::move(value);
		DescriptorBase::valid = false;
	}
	
	static constexpr VkDescriptorSetLayoutBinding layoutBinding = {
		.binding = binding,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
	
	std::optional<VkWriteDescriptorSet> DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override {
		if(!object){
			return {};
		}
		
		bufferInfoBuffer[bufferInfoBufferIndex].buffer = object->BufferFlying(PositiveModulo(flight + flightOffset, MAX_FRAMES_IN_FLIGHT));
		bufferInfoBuffer[bufferInfoBufferIndex].offset = 0;
		bufferInfoBuffer[bufferInfoBufferIndex].range = object->Size();
		
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
	
	static constexpr VkDescriptorPoolSize poolSize = {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1 * MAX_FRAMES_IN_FLIGHT
	};
	
private:
	std::shared_ptr<StorageBufferObject> object {};
	int flightOffset;
};

} // namespace EVK

