#include <Base.hpp>

namespace EVK {

Interface::Pipeline::DescriptorSet::DescriptorSet(Pipeline &_pipeline, int _index, const DescriptorSetBlueprint &blueprint) : pipeline(_pipeline), index(_index), descriptorsN(blueprint.descriptorsN){
	
	descriptors = (Descriptor **)malloc(blueprint.descriptorsN*sizeof(Descriptor *));
	for(int i=0; i<blueprint.descriptorsN; i++){
		switch(blueprint.descriptorBlueprints[i].type){
			case DescriptorType::UBO: {
				UniformBufferObject &ref = _pipeline.vulkan.uniformBufferObjects[blueprint.descriptorBlueprints[i].indicesA[0]];
				uboDynamic = ref.dynamicRepeats > 1;
				if(uboDynamic) uboDynamicAlignment = ref.dynamicAlignment;
				descriptors[i] = new UBODescriptor(*this, blueprint.descriptorBlueprints[i].binding, blueprint.descriptorBlueprints[i].stageFlags, blueprint.descriptorBlueprints[i].indicesA[0]);
				break;
			}
			case DescriptorType::SBO:
				descriptors[i] = new SBODescriptor(*this, blueprint.descriptorBlueprints[i].binding, blueprint.descriptorBlueprints[i].stageFlags, blueprint.descriptorBlueprints[i].indicesA[0]);
				break;
			case DescriptorType::textureImage:
				descriptors[i] = new TextureImagesDescriptor(*this, blueprint.descriptorBlueprints[i].binding, blueprint.descriptorBlueprints[i].stageFlags, blueprint.descriptorBlueprints[i].count, blueprint.descriptorBlueprints[i].indicesA);
				break;
			case DescriptorType::textureSampler:
				descriptors[i] = new TextureSamplersDescriptor(*this, blueprint.descriptorBlueprints[i].binding, blueprint.descriptorBlueprints[i].stageFlags, blueprint.descriptorBlueprints[i].count, blueprint.descriptorBlueprints[i].indicesA);
				break;
			case DescriptorType::combinedImageSampler:
				descriptors[i] = new CombinedImageSamplersDescriptor(*this, blueprint.descriptorBlueprints[i].binding, blueprint.descriptorBlueprints[i].stageFlags, blueprint.descriptorBlueprints[i].count, blueprint.descriptorBlueprints[i].indicesA, blueprint.descriptorBlueprints[i].indicesB);
				break;
			case DescriptorType::storageImage:
				descriptors[i] = new StorageImagesDescriptor(*this, blueprint.descriptorBlueprints[i].binding, blueprint.descriptorBlueprints[i].stageFlags, blueprint.descriptorBlueprints[i].count, blueprint.descriptorBlueprints[i].indicesA);
				break;
			default:
				throw std::runtime_error("unhandled descriptor type!");
		}
	}
}


void Interface::Pipeline::DescriptorSet::InitLayouts(){
#ifdef MAKE_ASSERTIONS
	//assert(descriptorCount == descriptorsArraySize);
#endif
	VkDescriptorSetLayoutBinding layoutBindings[descriptorsN];
	for(int i=0; i<descriptorsN; i++) layoutBindings[i] = descriptors[i]->LayoutBinding();
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = descriptorsN;
	layoutInfo.pBindings = layoutBindings;
	if(vkCreateDescriptorSetLayout(pipeline.vulkan.devices.logicalDevice, &layoutInfo, nullptr, &pipeline.descriptorSetLayouts[index]) != VK_SUCCESS) throw std::runtime_error("failed to create descriptor set layout!");
}

void Interface::Pipeline::DescriptorSet::InitConfigurations(){
	VkDescriptorBufferInfo bufferInfos[32];
	VkDescriptorImageInfo imageInfos[256];
	int bufferInfoCounter, imageInfoCounter;
	
	// configuring descriptors in descriptor sets
	for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
		VkWriteDescriptorSet *descriptorWrites = (VkWriteDescriptorSet *)malloc(descriptorsN*sizeof(VkWriteDescriptorSet));
		
		bufferInfoCounter = 0;
		imageInfoCounter = 0;
		
		for(int j=0; j<descriptorsN; j++) descriptorWrites[j] = descriptors[j]->DescriptorWrite(pipeline.descriptorSetsFlying[pipeline.descriptorSetsN*i + index], imageInfos, imageInfoCounter, bufferInfos, bufferInfoCounter, i);
		
		vkUpdateDescriptorSets(pipeline.vulkan.devices.logicalDevice, descriptorsN, descriptorWrites, 0, nullptr);
		free(descriptorWrites);
	}
}

} // namespace::EVK
