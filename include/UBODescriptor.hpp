#pragma once

#include "DescriptorBase.hpp"

namespace EVK {

template <uint32_t binding, VkShaderStageFlags stageFlags, typename T, bool dynamic>
class UBODescriptor : public DescriptorBase<binding, stageFlags> {
public:
	using DescriptorBase = typename UBODescriptor::DescriptorBase;
	static constexpr uint32_t bindingValue = binding;
	static constexpr VkShaderStageFlags stageFlagsValue = stageFlags;
	
	UBODescriptor() = default;
	
	void Set(const std::shared_ptr<UniformBufferObject<T, dynamic>> &value){
		object = value;
		DescriptorBase::valid = false;
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
	
	std::optional<DynamicUBOInfo> GetUBODynamic() const override {
		if(!object){
			std::cout << "Cannot get if UBO is dynamic; no object.\n";
			return {};
		}
		return object->GetDynamic();
	}
	
private:
	std::shared_ptr<UniformBufferObject<T, dynamic>> object {};
};

} // namespace EVK
