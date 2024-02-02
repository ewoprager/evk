#include <Base.hpp>

namespace EVK {

void Interface::GraphicsPipeline::Bind(){
	vkCmdBindPipeline(vulkan.commandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}
void Interface::ComputePipeline::Bind(){
	vkCmdBindPipeline(vulkan.computeCommandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
}
void Interface::GraphicsPipeline::BindDescriptorSets(int first, int number, const std::vector<int> &dynamicOffsetNumbers){
	if(!dynamicOffsetNumbers.empty()){
		std::vector<uint32_t> theseDynamicOffsets {};
		int indexOfDynamic = 0;
		for(int i=first; i<first + number; i++) if(descriptorSets[i]->GetUBODynamic()){
			theseDynamicOffsets.push_back((uint32_t)(dynamicOffsetNumbers[indexOfDynamic] * descriptorSets[i]->GetUBODynamicAlignment().value()));
		}
		assert(theseDynamicOffsets.size() == dynamicOffsetNumbers.size());
		vkCmdBindDescriptorSets(vulkan.commandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, first, number, &descriptorSetsFlying[descriptorSets.size() * vulkan.currentFrame], uint32_t(dynamicOffsetNumbers.size()), theseDynamicOffsets.data());
	} else {
		vkCmdBindDescriptorSets(vulkan.commandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, first, number, &descriptorSetsFlying[descriptorSets.size() * vulkan.currentFrame], 0, nullptr);
	}
}
void Interface::ComputePipeline::BindDescriptorSets(int first, int number, const std::vector<int> &dynamicOffsetNumbers){
	if(!dynamicOffsetNumbers.empty()){
		std::vector<uint32_t> theseDynamicOffsets {};
		int indexOfDynamic = 0;
		for(int i=first; i<first + number; i++) if(descriptorSets[i]->GetUBODynamic()){
			theseDynamicOffsets.push_back((uint32_t)(dynamicOffsetNumbers[indexOfDynamic] * descriptorSets[i]->GetUBODynamicAlignment().value()));
		}
		assert(theseDynamicOffsets.size() == dynamicOffsetNumbers.size());
		vkCmdBindDescriptorSets(vulkan.computeCommandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, layout, first, number, &descriptorSetsFlying[descriptorSets.size() * vulkan.currentFrame], uint32_t(dynamicOffsetNumbers.size()), theseDynamicOffsets.data());
	} else {
		vkCmdBindDescriptorSets(vulkan.computeCommandBuffersFlying[vulkan.currentFrame], VK_PIPELINE_BIND_POINT_COMPUTE, layout, first, number, &descriptorSetsFlying[descriptorSets.size() * vulkan.currentFrame], 0, nullptr);
	}
}

Interface::Pipeline::Pipeline(Interface &_vulkan, const PipelineBlueprint &blueprint) : vulkan(_vulkan), pushConstantRanges(blueprint.pushConstantRanges) {
	
	descriptorSets = std::vector<std::shared_ptr<DescriptorSet>>(blueprint.descriptorSetBlueprints.size());
	for(int i=0; i<blueprint.descriptorSetBlueprints.size(); ++i)
		descriptorSets[i] = std::make_shared<DescriptorSet>(*this, i, blueprint.descriptorSetBlueprints[i]);
	
	descriptorSetLayouts = std::vector<VkDescriptorSetLayout>(blueprint.descriptorSetBlueprints.size());
	descriptorSetsFlying = std::vector<VkDescriptorSet>(blueprint.descriptorSetBlueprints.size() * MAX_FRAMES_IN_FLIGHT);
	
	uint32_t descriptorSetLayoutBindingNumber = 0;
	for(const DescriptorSetBlueprint &descriptorSetBlueprint : blueprint.descriptorSetBlueprints){
		descriptorSetLayoutBindingNumber += descriptorSetBlueprint.size();
	}
	
	std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(descriptorSetLayoutBindingNumber);
	int count = 0;
	for(int si=0; si<blueprint.descriptorSetBlueprints.size(); ++si){
		for(int di=0; di<blueprint.descriptorSetBlueprints[si].size(); ++di){
			poolSizes[count] = descriptorSets[si]->GetDescriptor(di)->PoolSize();
			poolSizes[count].descriptorCount *= MAX_FRAMES_IN_FLIGHT;
			++count;
		}
	}
	assert(count == descriptorSetLayoutBindingNumber);
	
	VkDescriptorPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = descriptorSetLayoutBindingNumber,
		.pPoolSizes = poolSizes.data(),
		.maxSets = (uint32_t)(MAX_FRAMES_IN_FLIGHT * blueprint.descriptorSetBlueprints.size())
	};
	if(vkCreateDescriptorPool(vulkan.devices.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");
	
	
	// -----
	// Initialising descriptor set layouts
	// -----
	for(std::shared_ptr<DescriptorSet> descriptorSet : descriptorSets) descriptorSet->InitLayouts();
	
	
	// Allocating descriptor sets
	{
		const uint32_t n = (uint32_t)(blueprint.descriptorSetBlueprints.size() * MAX_FRAMES_IN_FLIGHT);
		VkDescriptorSetLayout flyingLayouts[n];
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) for(int j=0; j<blueprint.descriptorSetBlueprints.size(); j++){
			flyingLayouts[blueprint.descriptorSetBlueprints.size() * i + j] = descriptorSetLayouts[j];
		}
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = n,
			.pSetLayouts = flyingLayouts
		};
		if(vkAllocateDescriptorSets(vulkan.devices.logicalDevice, &allocInfo, descriptorSetsFlying.data()) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate descriptor sets!");
	}

}
void Interface::Pipeline::UpdateDescriptorSets(uint32_t first){
	for(uint32_t i=first; i<descriptorSets.size(); ++i) descriptorSets[i]->Update();
}
Interface::GraphicsPipeline::GraphicsPipeline(Interface &_vulkan, const GraphicsPipelineBlueprint &blueprint) : Pipeline(_vulkan, blueprint.pipelineBlueprint){
	
	// ----- Input assembly info -----
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = blueprint.primitiveTopology,
		.primitiveRestartEnable = VK_FALSE
	};
	
	// ----- Viewport state -----
	VkPipelineViewportStateCreateInfo viewportState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};
	
	// -----
	// Creating the layout
	// -----
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(descriptorSetLayouts.size()), // Optional
		.pSetLayouts = descriptorSetLayouts.data(), // Optional
		.pushConstantRangeCount = uint32_t(blueprint.pipelineBlueprint.pushConstantRanges.size()),
		.pPushConstantRanges = blueprint.pipelineBlueprint.pushConstantRanges.empty() ? nullptr : pushConstantRanges.data()
	};
	if(vkCreatePipelineLayout(vulkan.devices.logicalDevice, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");
	
	
	// -----
	// Creating the pipeline
	// -----
	VkGraphicsPipelineCreateInfo pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = uint32_t(blueprint.shaderStageCIs.size()),
		.pStages = blueprint.shaderStageCIs.data(),
		.pVertexInputState = &blueprint.vertexInputStateCI,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &blueprint.rasterisationStateCI,
		.pMultisampleState = &blueprint.multisampleStateCI,
		.pDepthStencilState = &blueprint.depthStencilStateCI, // Optional
		.pColorBlendState = &blueprint.colourBlendStateCI,
		.pDynamicState = &blueprint.dynamicStateCI,
		.layout = layout,
		.renderPass = (blueprint.bufferedRenderPassIndex
					   ? vulkan.GetBufferedRenderPassHandle(blueprint.bufferedRenderPassIndex.value())
					   : (blueprint.layeredBufferedRenderPassIndex
						  ? vulkan.GetLayeredBufferedRenderPassHandle(blueprint.layeredBufferedRenderPassIndex.value())
						  : vulkan.renderPass
						  )
					   ),
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // Optional
		.basePipelineIndex = -1 // Optional
	};
	if(vkCreateGraphicsPipelines(vulkan.devices.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

}
Interface::ComputePipeline::ComputePipeline(Interface &_vulkan, const ComputePipelineBlueprint &blueprint) : Pipeline(_vulkan, blueprint.pipelineBlueprint){
	// -----
	// Creating the layout
	// -----
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32_t(blueprint.pipelineBlueprint.descriptorSetBlueprints.size()), // Optional
		.pSetLayouts = descriptorSetLayouts.data(), // Optional
		.pushConstantRangeCount = uint32_t(blueprint.pipelineBlueprint.pushConstantRanges.size()),
		.pPushConstantRanges = blueprint.pipelineBlueprint.pushConstantRanges.empty() ? nullptr : pushConstantRanges.data()
	};
	if(vkCreatePipelineLayout(vulkan.devices.logicalDevice, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");
	
	// -----
	// Creating the pipeline
	// -----
	VkComputePipelineCreateInfo pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = blueprint.shaderStageCI,
		.layout = layout
	};
	if(vkCreateComputePipelines(vulkan.devices.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create compute pipeline!");

}

} // namespace::EVK
