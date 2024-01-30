#include <Base.hpp>

namespace EVK {

Interface::Pipeline::DescriptorSet::DescriptorSet(Pipeline &_pipeline, int _index, const DescriptorSetBlueprint &blueprint) : pipeline(_pipeline), index(_index) {
	
	descriptors = std::vector<std::shared_ptr<Descriptor>>(blueprint.size());
	for(int i=0; i<blueprint.size(); i++){
		switch(blueprint[i].type){
			case DescriptorType::UBO: {
				if(!_pipeline.vulkan.uniformBufferObjects[blueprint[i].indicesExtra[0]])
					throw std::runtime_error("Cannot make descriptor set as UBO hasn't been created.");
				UniformBufferObject &ref = _pipeline.vulkan.uniformBufferObjects[blueprint[i].indicesExtra[0]].value();
				if(ref.dynamic){
					uboDynamicAlignment = ref.dynamic->alignment;
				} else {
					uboDynamicAlignment.reset();
				}
				descriptors[i] = std::make_shared<UBODescriptor>(*this, blueprint[i].binding, blueprint[i].stageFlags, blueprint[i].indicesExtra[0]);
				break;
			}
			case DescriptorType::SBO:
				descriptors[i] = std::make_shared<SBODescriptor>(*this, blueprint[i].binding, blueprint[i].stageFlags, blueprint[i].indicesExtra[0], blueprint[i].indicesExtra[1]);
				break;
			case DescriptorType::textureImage:
				descriptors[i] = std::make_shared<TextureImagesDescriptor>(*this, blueprint[i].binding, blueprint[i].stageFlags, blueprint[i].indicesExtra);
				break;
			case DescriptorType::textureSampler:
				descriptors[i] = std::make_shared<TextureSamplersDescriptor>(*this, blueprint[i].binding, blueprint[i].stageFlags, blueprint[i].indicesExtra);
				break;
			case DescriptorType::combinedImageSampler:
				descriptors[i] = std::make_shared<CombinedImageSamplersDescriptor>(*this, blueprint[i].binding, blueprint[i].stageFlags, blueprint[i].indicesExtra, blueprint[i].indicesExtra2);
				break;
			case DescriptorType::storageImage:
				descriptors[i] = std::make_shared<StorageImagesDescriptor>(*this, blueprint[i].binding, blueprint[i].stageFlags, blueprint[i].indicesExtra);
				break;
			default:
				throw std::runtime_error("unhandled descriptor type!");
		}
	}
}


void Interface::Pipeline::DescriptorSet::InitLayouts(){
	VkDescriptorSetLayoutBinding layoutBindings[descriptors.size()];
	for(int i=0; i<descriptors.size(); i++) layoutBindings[i] = descriptors[i]->LayoutBinding();
	
	VkDescriptorSetLayoutCreateInfo layoutInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = uint32_t(descriptors.size()),
		.pBindings = layoutBindings
	};
	if(vkCreateDescriptorSetLayout(pipeline.vulkan.devices.logicalDevice, &layoutInfo, nullptr, &pipeline.descriptorSetLayouts[index]) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");
}

void Interface::Pipeline::DescriptorSet::InitConfigurations(){
	VkDescriptorBufferInfo bufferInfos[32];
	VkDescriptorImageInfo imageInfos[256];
	int bufferInfoCounter, imageInfoCounter;
	
	// configuring descriptors in descriptor sets
	for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
		std::vector<VkWriteDescriptorSet> descriptorWrites = std::vector<VkWriteDescriptorSet>(descriptors.size());
		
		bufferInfoCounter = 0;
		imageInfoCounter = 0;
		
		for(int j=0; j<descriptors.size(); j++)
			descriptorWrites[j] = descriptors[j]->DescriptorWrite(pipeline.descriptorSetsFlying[pipeline.descriptorSets.size() * i + index], imageInfos, imageInfoCounter, bufferInfos, bufferInfoCounter, i);
		
		vkUpdateDescriptorSets(pipeline.vulkan.devices.logicalDevice, uint32_t(descriptors.size()), descriptorWrites.data(), 0, nullptr);
	}
}

} // namespace::EVK
