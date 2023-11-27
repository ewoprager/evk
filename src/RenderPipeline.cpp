#include <Base.hpp>

namespace EVK {

void Interface::GraphicsPipeline::Bind(){
	vkCmdBindPipeline(vulkan.commandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}
void Interface::ComputePipeline::Bind(){
	vkCmdBindPipeline(vulkan.commandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
}
void Interface::GraphicsPipeline::BindDescriptorSets(const int &first, const int &number, const uint32_t &dynamicOffsetCount, const int *const &dynamicOffsetNumbers){
	if(dynamicOffsetCount){
#ifdef MAKE_ASSERTIONS
		assert(dynamicOffsetNumbers);
#endif
		int dynamicOffsetCount = 0;
		for(int i=first; i<first + number; i++) dynamicOffsetCount += (int)descriptorSets[i]->GetUBODynamic();
		uint32_t newOffsets[dynamicOffsetCount];
		int i_ = 0;
		for(int i=first; i<first + number; i++) if(descriptorSets[i]->GetUBODynamic()){
			newOffsets[i_] = (uint32_t)(dynamicOffsetNumbers[i_]*descriptorSets[i]->GetUBODynamicAlignment());
			i_++;
		}
		
		vkCmdBindDescriptorSets(vulkan.commandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, first, number, &descriptorSetsFlying[descriptorSetsN*vulkan.currentFrame], dynamicOffsetCount, newOffsets);
	} else {
		vkCmdBindDescriptorSets(vulkan.commandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, first, number, &descriptorSetsFlying[descriptorSetsN*vulkan.currentFrame], 0, nullptr);
	}
}
void Interface::ComputePipeline::BindDescriptorSets(const int &first, const int &number, const uint32_t &dynamicOffsetCount, const int *const &dynamicOffsetNumbers){
	if(dynamicOffsetCount){
#ifdef MAKE_ASSERTIONS
		assert(dynamicOffsetNumbers);
#endif
		int dynamicOffsetCount = 0;
		for(int i=first; i<first + number; i++) dynamicOffsetCount += (int)descriptorSets[i]->GetUBODynamic();
		uint32_t newOffsets[dynamicOffsetCount];
		int i_ = 0;
		for(int i=first; i<first + number; i++) if(descriptorSets[i]->GetUBODynamic()){
			newOffsets[i_] = (uint32_t)(dynamicOffsetNumbers[i_]*descriptorSets[i]->GetUBODynamicAlignment());
			i_++;
		}
		
		vkCmdBindDescriptorSets(vulkan.commandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, layout, first, number, &descriptorSetsFlying[descriptorSetsN*vulkan.currentFrame], dynamicOffsetCount, newOffsets);
	} else {
		vkCmdBindDescriptorSets(vulkan.commandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, layout, first, number, &descriptorSetsFlying[descriptorSetsN*vulkan.currentFrame], 0, nullptr);
	}
}

Interface::Pipeline::Pipeline(Interface &_vulkan, const PipelineBlueprint &blueprint) : vulkan(_vulkan) {
	
	pushConstantRanges = (VkPushConstantRange *)malloc(blueprint.pushConstantRangesN*sizeof(VkPushConstantRange));
	memcpy(pushConstantRanges, blueprint.pushConstantRanges, blueprint.pushConstantRangesN*sizeof(VkPushConstantRange));
	pushConstantRangesN = blueprint.pushConstantRangesN;
	
	descriptorSets = (DescriptorSet **)malloc(blueprint.descriptorSetsN*sizeof(DescriptorSet *));
	for(int i=0; i<blueprint.descriptorSetsN; i++) descriptorSets[i] = new DescriptorSet(*this, i, blueprint.descriptorSetBlueprints[i]);
	descriptorSetsN = blueprint.descriptorSetsN;
	
	descriptorSetLayouts = (VkDescriptorSetLayout *)malloc(blueprint.descriptorSetsN*sizeof(VkDescriptorSetLayout));
	descriptorSetsFlying = (VkDescriptorSet *)malloc(blueprint.descriptorSetsN*MAX_FRAMES_IN_FLIGHT*sizeof(VkDescriptorSet));
	
	int descriptorSetLayoutBindingNumber = 0;
	for(int i=0; i<blueprint.descriptorSetsN; i++) descriptorSetLayoutBindingNumber += blueprint.descriptorSetBlueprints[i].descriptorsN;
	
	VkDescriptorPoolSize *poolSizes = (VkDescriptorPoolSize *)malloc(descriptorSetLayoutBindingNumber*sizeof(VkDescriptorPoolSize));
	int count = 0;
	for(int si=0; si<blueprint.descriptorSetsN; si++){
		for(int di=0; di<blueprint.descriptorSetBlueprints[si].descriptorsN; di++){
			poolSizes[count] = descriptorSets[si]->GetDescriptor(di)->PoolSize();
			poolSizes[count].descriptorCount *= MAX_FRAMES_IN_FLIGHT;
			count++;
		}
	}
#ifdef MAKE_ASSERTIONS
	assert(count == descriptorSetLayoutBindingNumber);
#endif
	
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = descriptorSetLayoutBindingNumber;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = (uint32_t)(MAX_FRAMES_IN_FLIGHT*blueprint.descriptorSetsN);
	if(vkCreateDescriptorPool(vulkan.devices.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) throw std::runtime_error("failed to create descriptor pool!");
	free(poolSizes);
	
	
	// -----
	// Initialising descriptor set layouts
	// -----
	for(int i=0; i<blueprint.descriptorSetsN; i++) descriptorSets[i]->InitLayouts();
	
	
	// Allocating descriptor sets
	{
		const uint32_t n = (uint32_t)(blueprint.descriptorSetsN*MAX_FRAMES_IN_FLIGHT);
		VkDescriptorSetLayout flyingLayouts[n];
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) for(int j=0; j<blueprint.descriptorSetsN; j++){
			flyingLayouts[blueprint.descriptorSetsN*i + j] = descriptorSetLayouts[j];
		}
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = n;
		allocInfo.pSetLayouts = flyingLayouts;
		if(vkAllocateDescriptorSets(vulkan.devices.logicalDevice, &allocInfo, descriptorSetsFlying) != VK_SUCCESS) throw std::runtime_error("failed to allocate descriptor sets!");
	}
	
	// -----
	// Initialising descriptor set configurations
	// -----
	for(int i=0; i<blueprint.descriptorSetsN; i++) descriptorSets[i]->InitConfigurations();
}
void Interface::Pipeline::UpdateDescriptorSets(){
	for(int i=0; i<descriptorSetsN; i++) descriptorSets[i]->InitConfigurations();
}
Interface::GraphicsPipeline::GraphicsPipeline(Interface &_vulkan, const GraphicsPipelineBlueprint &blueprint) : Pipeline(_vulkan, blueprint.pipelineBlueprint){
	
	// ----- Input assembly info -----
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	
	
	// ----- Viewport state -----
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	
	
	// -----
	// Creating the layout
	// -----
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = blueprint.pipelineBlueprint.descriptorSetsN; // Optional
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = blueprint.pipelineBlueprint.pushConstantRangesN;
	pipelineLayoutInfo.pPushConstantRanges = blueprint.pipelineBlueprint.pushConstantRangesN ? pushConstantRanges : nullptr;
	if(vkCreatePipelineLayout(vulkan.devices.logicalDevice, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) throw std::runtime_error("failed to create pipeline layout!");
	
	
	// -----
	// Creating the pipeline
	// -----
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = blueprint.stageCount;
	pipelineInfo.pStages = blueprint.shaderStageCIs;
	pipelineInfo.pVertexInputState = &blueprint.vertexInputStateCI;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &blueprint.rasterisationStateCI;
	pipelineInfo.pMultisampleState = &blueprint.multisampleStateCI;
	pipelineInfo.pDepthStencilState = &blueprint.depthStencilStateCI; // Optional
	pipelineInfo.pColorBlendState = &blueprint.colourBlendStateCI;
	pipelineInfo.pDynamicState = &blueprint.dynamicStateCI;
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = blueprint.bufferedRenderPassIndex < 0 ? (blueprint.layeredBufferedRenderPassIndex < 0 ? vulkan.renderPass : vulkan.GetLayeredBufferedRenderPassHandle(blueprint.layeredBufferedRenderPassIndex)) : vulkan.GetBufferedRenderPassHandle(blueprint.bufferedRenderPassIndex);
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if(vkCreateGraphicsPipelines(vulkan.devices.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) throw std::runtime_error("failed to create graphics pipeline!");

}
Interface::ComputePipeline::ComputePipeline(Interface &_vulkan, const ComputePipelineBlueprint &blueprint) : Pipeline(_vulkan, blueprint.pipelineBlueprint){
	
	// -----
	// Creating the layout
	// -----
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = blueprint.pipelineBlueprint.descriptorSetsN; // Optional
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = blueprint.pipelineBlueprint.pushConstantRangesN;
	pipelineLayoutInfo.pPushConstantRanges = blueprint.pipelineBlueprint.pushConstantRangesN ? pushConstantRanges : nullptr;
	if(vkCreatePipelineLayout(vulkan.devices.logicalDevice, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) throw std::runtime_error("failed to create pipeline layout!");
	
	
	// -----
	// Creating the pipeline
	// -----
	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = blueprint.shaderStageCI;
	pipelineInfo.layout = layout;
	if(vkCreateComputePipelines(vulkan.devices.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) throw std::runtime_error("failed to create compute pipeline!");

}

} // namespace::EVK
