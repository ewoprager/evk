#pragma once

#include "Static.hpp"

#include "Devices.hpp"

#include "UBO.hpp"
#include "SBO.hpp"
#include "TextureImages.hpp"
#include "TextureSamplers.hpp"
#include "CombinedImageSamplers.hpp"
#include "StorageImages.hpp"

namespace EVK {

// Push constants
// -----
struct NoPushConstants {};
template <size_t offset, typename T> struct PushConstants {
	static constexpr uint32_t offsetValue = offset;
	using type = T;
};
template <VkShaderStageFlags stageFlags, typename T> struct WithShaderStage {
	static constexpr VkShaderStageFlags stageFlagsValue = stageFlags;
	using type = T;
};
template <VkShaderStageFlags stageFlags, size_t offset, typename T>
struct WithShaderStage<stageFlags, PushConstants<offset, T>> {
	static constexpr VkPushConstantRange rangeValue = {stageFlags, offset, sizeof(T)};
};

// merge one WithShaderStage into a pack of pre-merged WithShaderStages
template <typename checkedTP, typename withShaderStageT, typename notCheckedTP> struct MergeIn {};
template <typename... checkedTs, typename withShaderStageT> struct MergeIn<TypePack<checkedTs...>, withShaderStageT, TypePack<>> {
	using packType = TypePack<checkedTs..., withShaderStageT>;
};
template <typename... checkedTs, typename withShaderStageT, typename notCheckedT> struct MergeIn<TypePack<checkedTs...>, withShaderStageT, TypePack<notCheckedT>> {
	using packType = std::conditional_t<
	std::same_as<typename withShaderStageT::type, typename notCheckedT::type>,
	TypePack<checkedTs..., WithShaderStage<withShaderStageT::stageFlagsValue | notCheckedT::stageFlagsValue, typename withShaderStageT::type>>,
	TypePack<checkedTs..., withShaderStageT, notCheckedT>
	>;
};
template <typename... checkedTs, typename withShaderStageT, typename notCheckedT, typename other, typename... rest> struct MergeIn<TypePack<checkedTs...>, withShaderStageT, TypePack<notCheckedT, other, rest...>> {
	using packType = std::conditional_t<
	std::same_as<typename withShaderStageT::type, typename notCheckedT::type>,
	TypePack<checkedTs..., WithShaderStage<withShaderStageT::stageFlagsValue | notCheckedT::stageFlagsValue, typename withShaderStageT::type>, other, rest...>,
	typename MergeIn<TypePack<checkedTs..., notCheckedT>, withShaderStageT, TypePack<other, rest...>>::packType
	>;
};
template <typename withShaderStageT, typename mergedTP>
using MergeIn_t = typename MergeIn<TypePack<>, withShaderStageT, mergedTP>::packType;

// merge all of a pack of WithShaderStages together
template <typename mergedTP, typename unmergedTP> struct Merge {};
template <typename... mergedTs> struct Merge<TypePack<mergedTs...>, TypePack<>> {
	using packType = TypePack<mergedTs...>;
};
template <typename... mergedTs, typename unmergedT> struct Merge<TypePack<mergedTs...>, TypePack<unmergedT>> {
	using packType = MergeIn_t<unmergedT, TypePack<mergedTs...>>;
};
template <typename... mergedTs, typename unmergedT, typename U, typename... rest> struct Merge<TypePack<mergedTs...>, TypePack<unmergedT, U, rest...>> {
	using packType = typename Merge<MergeIn_t<unmergedT, TypePack<mergedTs...>>, TypePack<U, rest...>>::packType;
};
template <typename withShaderStageTP>
using Merge_t = typename Merge<TypePack<>, withShaderStageTP>::packType;


// Uniform
// -----
template <uint32_t set, uint32_t binding> struct UniformBase {
	static constexpr uint32_t setValue = set;
	static constexpr uint32_t bindingValue = binding;
};

template <uint32_t set, uint32_t binding, bool dynamic=false>
struct UBO : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptorType = UBODescriptor<binding, stageFlags, dynamic>;
};
template <uint32_t set, uint32_t binding>
struct SBO : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptorType = SBODescriptor<binding, stageFlags>;
};
template <uint32_t set, uint32_t binding, uint32_t count=1>
struct TextureImages : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptorType = TextureImagesDescriptor<binding, stageFlags, count>;
};
template <uint32_t set, uint32_t binding, uint32_t count=1>
struct TextureSamplers : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptorType = TextureSamplersDescriptor<binding, stageFlags, count>;
};
template <uint32_t set, uint32_t binding, uint32_t count=1>
struct CombinedImageSamplers : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptorType = CombinedImageSamplersDescriptor<binding, stageFlags, count>;
};
template <uint32_t set, uint32_t binding, uint32_t count=1>
struct StorageImages : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptorType = StorageImagesDescriptor<binding, stageFlags, count>;
};

template <typename uniformT, typename... uniformTs> consteval bool UniformContains(){
	return ((uniformT::type::setValue == uniformTs::type::setValue && uniformT::type::bindingValue == uniformTs::type::bindingValue) || ...);
}
template <typename uniformT> consteval bool UniformUnique(){ return true; }
template <typename uniformT1, typename uniformT2, typename... uniformTs> consteval bool UniformUnique(){
	return !UniformContains<uniformT1, uniformT2, uniformTs...>() && UniformUnique<uniformT2, uniformTs...>();
}

template <uint32_t set, typename uniformT>
using FilteredDescriptorPackOne = std::conditional_t<
uniformT::type::setValue == set,
TypePack<uniformT::type::descriptorType<uniformT::stageFlagsValue>>,
TypePack<>
>;

template <uint32_t set, typename... uniformTs>
using FilteredDescriptorPack = ConcatenatedPack_t<FilteredDescriptorPackOne<set, uniformTs>...>;

template <uint32_t set, typename... uniformTs> static consteval
bool DescriptorSetExistsInUniforms(){ return !FilteredDescriptorPack<set, uniformTs...>::empty; }

template <uint32_t set, typename... uniformTs> static consteval
uint32_t DescriptorSetCountImpl(){
	if constexpr (DescriptorSetExistsInUniforms<set, uniformTs...>())
		return DescriptorSetCountImpl<set + 1, uniformTs...>();
	else
		return set;
}

template <typename... uniformTs> static constexpr
uint32_t DescriptorSetCount(){ return DescriptorSetCountImpl<0, uniformTs...>(); }

template <uint32_t set, typename... uniformTs> static consteval
bool AllSmallerSetsExistInUniforms(){
	if constexpr (set == 0){
		return DescriptorSetExistsInUniforms<0, uniformTs...>();
	} else {
		return DescriptorSetExistsInUniforms<set, uniformTs...>() && AllSmallerSetsExistInUniforms<set - 1, uniformTs...>();
	}
}


// Attributes
// -----
template <VkVertexInputBindingDescription... bindingDescriptions> struct BindingDescriptionPack {};
template <VkVertexInputAttributeDescription... attributeDescriptions> struct AttributeDescriptionPack {};

template <typename attributeTP, typename bindingDescriptionP, typename attributeDescriptionP> struct Attributes {};
template <typename... attributeTs, VkVertexInputBindingDescription... bindingDescriptions, VkVertexInputAttributeDescription... attributeDescriptions>
struct Attributes<TypePack<attributeTs...>, BindingDescriptionPack<bindingDescriptions...>, AttributeDescriptionPack<attributeDescriptions...>> {
	
	static constexpr uint32_t bindingDescriptionCount = CountT<bindingDescriptions...>();
	
	static constexpr uint32_t attributeDescriptionCount = CountT<attributeDescriptions...>();
	
	static consteval uint32_t TotalSize(){ return (sizeof(attributeTs) + ...); }
	
	static_assert((bindingDescriptions.stride + ...) == TotalSize(), "Vertex input binding description strides should sum to the total size of all the attributes.");
	
	static_assert(Unique<bindingDescriptions.binding...>(), "Vertex input binding description bindings should be unique.");
	
	static_assert(Unique<attributeDescriptions.location...>(), "Vertex input attribute description locations should be unique.");
	
//	template <typename T> void BindVertexBuffer(){
//		
//	}
	
	static constexpr std::array<VkVertexInputBindingDescription, bindingDescriptionCount> bindingDescriptionsValue = {(bindingDescriptions, ...)};
	static constexpr std::array<VkVertexInputAttributeDescription, attributeDescriptionCount> attributeDescriptionsValue = {(attributeDescriptions, ...)};
	static constexpr VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCI = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.vertexBindingDescriptionCount = bindingDescriptionCount,
		.pVertexBindingDescriptions = bindingDescriptionsValue.data(),
		.vertexAttributeDescriptionCount = attributeDescriptionCount,
		.pVertexAttributeDescriptions = attributeDescriptionsValue.data()
	};
};


// Descriptor set
// -----
template <typename descriptorTP> struct DescriptorSet {};
template <typename... descriptorTs>
struct DescriptorSet<TypePack<descriptorTs...>> {
	static constexpr uint32_t descriptorCount = CountT<descriptorTs...>();
	
	template <uint32_t index> using iDescriptor = IndexT<index, descriptorTs...>;
	
	explicit DescriptorSet(std::shared_ptr<Devices> _devices)
	: devices(_devices) {}
	
	VkDescriptorSetLayout CreateLayout() const {
		constexpr std::array<VkDescriptorSetLayoutBinding, descriptorCount> layoutBindings = {(descriptorTs::LayoutBinding(), ...)};
		// binding flags
		std::array<VkDescriptorBindingFlags, descriptorCount> flags{};
		std::ranges::fill(flags, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
		const VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.pNext = nullptr,
			.pBindingFlags = flags.data(),
			.bindingCount = descriptorCount
		};
		const VkDescriptorSetLayoutCreateInfo layoutInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &bindingFlags,
			.bindingCount = descriptorCount,
			.pBindings = layoutBindings.data()
		};
		VkDescriptorSetLayout handle;
		if(vkCreateDescriptorSetLayout(devices->GetLogicalDevice(), &layoutInfo, nullptr, &handle) != VK_SUCCESS){
			throw std::runtime_error("failed to create descriptor set layout!");
		}
		return handle;
	}
	
	void Update(const std::function<VkDescriptorSet(uint32_t)> &dstSetFromFlight){
		VkDescriptorBufferInfo bufferInfos[32];
		VkDescriptorImageInfo imageInfos[256];
		int bufferInfoCounter, imageInfoCounter;
		
		// configuring descriptors in descriptor sets
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
			std::vector<VkWriteDescriptorSet> descriptorWrites{};
			
			bufferInfoCounter = 0;
			imageInfoCounter = 0;
			
			[&]<uint32_t... indices>(std::index_sequence<indices...>){
				([&](){
					const std::optional<VkWriteDescriptorSet> maybeDescriptorWrite = std::get<indices>(descriptors).DescriptorWrite(dstSetFromFlight(i)/*pipeline.descriptorSetsFlying[pipeline.descriptorSets.size() * i + index]*/, imageInfos, imageInfoCounter, bufferInfos, bufferInfoCounter, i);
					if(maybeDescriptorWrite){
						descriptorWrites.push_back(maybeDescriptorWrite.value());
					}
				}(), ...);
			}(std::make_integer_sequence<uint32_t, descriptorCount>{});
			
			vkUpdateDescriptorSets(devices->GetLogicalDevice(), uint32_t(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
	
//	size_t GetDescriptorCount() const { return descriptors.size(); }
//	std::shared_ptr<Descriptor> GetDescriptor(int index) const { return descriptors[index]; }
//	bool GetUBODynamic() const { return uboDynamicAlignment.has_value(); }
//	const std::optional<VkDeviceSize> &GetUBODynamicAlignment() const { return uboDynamicAlignment; }
	
private:
	std::shared_ptr<Devices> devices;
	
	std::tuple<descriptorTs...> descriptors {};
	
	// ubo info
//	std::optional<VkDeviceSize> uboDynamicAlignment;
	
	// this still relevent? \/\/\/ -
	/*
	 `index` is the index of this descriptor set's layout in 'Vulkan::RenderPipeline::descriptorSetLayouts',
	 
	 `pipeline.descriptorSetNumber*flightIndex`,
	 `pipeline.descriptorSetNumber*flightIndex + 1`
	 ...
	 `pipeline.descriptorSetNumber*flightIndex + pipeline.descriptorSetNumber - 1`
	 are the indices of this descriptors flying sets in 'pipeline.descriptorSetsFlying'
	 */
	// /\/\/\ -
};


// Uniforms
// -----
template <typename uniformTP, typename indexSequenceT> struct UniformsImpl {};
template <typename... uniformTs, uint32_t... indices>
struct UniformsImpl<TypePack<uniformTs...>, std::integer_sequence<uint32_t, indices...>> {
	
	static_assert(UniformUnique<uniformTs...>(), "No two uniforms should have both matching set and binding.");
	
	static_assert((AllSmallerSetsExistInUniforms<uniformTs::type::setValue, uniformTs...>() && ...), "The union of descriptor set indices for a shader should be consecutive and contain 0.");
	
	template <uint32_t set> requires (DescriptorSetExistsInUniforms<set, uniformTs...>())
	using iDescriptorSet = DescriptorSet<FilteredDescriptorPack<set, uniformTs...>>;
	
	static constexpr uint32_t descriptorSetCount = DescriptorSetCount<uniformTs...>();
	
	static constexpr uint32_t uniformCount = CountT<uniformTs...>();
	static_assert((iDescriptorSet<indices>::descriptorCount + ...) == uniformCount, "!Inconsistency");
	
	static constexpr std::array<VkDescriptorPoolSize, uniformCount> poolSizes = {uniformTs::type::descriptorType<uniformTs::stageFlagsValue>>::PoolSize(), ...)};
	
	template <uint32_t index>
	iDescriptorSet<index> &DescriptorSet(){ return std::get<index>(descriptorSets); }
	
	explicit UniformsImpl(std::shared_ptr<Devices> _devices)
	: devices(_devices)
	, descriptorSets(iDescriptorSet<indices>(_devices)...) {
		// descriptor pool
		const VkDescriptorPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.poolSizeCount = uniformCount,
			.pPoolSizes = poolSizes.data(),
			.maxSets = uint32_t(MAX_FRAMES_IN_FLIGHT * descriptorSetCount)
		};
		if(vkCreateDescriptorPool(_devices->GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS){
			throw std::runtime_error("failed to create descriptor pool!");
		}
		
		// descriptor set layouts
		([&](){
			descriptorSetLayouts[indices] = std::get<indices>(descriptorSets).CreateLayout();
		}(), ...);
		
		// allocating descriptor sets
		VkDescriptorSetLayout flyingLayouts[descriptorSetCount * MAX_FRAMES_IN_FLIGHT];
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; ++i){
			for(int j=0; j<descriptorSetCount; ++j){
				flyingLayouts[descriptorSetCount * i + j] = descriptorSetLayouts[j];
			}
		}
		const VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = descriptorSetCount * MAX_FRAMES_IN_FLIGHT,
			.pSetLayouts = flyingLayouts
		};
		if(vkAllocateDescriptorSets(devices->GetLogicalDevice(), &allocInfo, descriptorSetsFlying.data()) != VK_SUCCESS){
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}
	~UniformsImpl(){
		vkDestroyDescriptorPool(devices->GetLogicalDevice(), descriptorPool, nullptr);
		for(VkDescriptorSetLayout &dsl : descriptorSetLayouts){
			vkDestroyDescriptorSetLayout(devices->GetLogicalDevice(), dsl, nullptr);
		}
	}
	
	void UpdateDescriptorSets(){
		(std::get<indices>(descriptorSets).Update([this](uint32_t flight) -> VkDescriptorSet {
			return descriptorSetsFlying[descriptorSetCount * flight + indices];
		}), ...);
	}
	
	const std::array<VkDescriptorSetLayout, descriptorSetCount> &DescriptorSetLayouts() const {
		return descriptorSetLayouts;
	}
	
	const VkDescriptorSet *DescriptorsSetsStart(int flight){
		return &descriptorSetsFlying[flight * descriptorSetCount];
	}
	
private:
	std::shared_ptr<Devices> devices;
	
	std::tuple<iDescriptorSet<indices>...> descriptorSets;
	
	std::array<VkDescriptorSetLayout, descriptorSetCount> descriptorSetLayouts {};
	std::array<VkDescriptorSet, descriptorSetCount * MAX_FRAMES_IN_FLIGHT> descriptorSetsFlying {};
	VkDescriptorPool descriptorPool;
};

template <typename... uniformTs>
using Uniforms = UniformsImpl<TypePack<uniformTs...>, std::make_integer_sequence<uint32_t, DescriptorSetCount<uniformTs...>()>>;


// Shader
// -----
template <VkShaderStageFlags shaderStage, typename filenameStringT, typename PushConstantsT, typename... uniformTs>
struct Shader {
	using pushConstantsPackType = std::conditional_t<
	std::same_as<PushConstantsT, NoPushConstants>,
	TypePack<>,
	TypePack<WithShaderStage<shaderStage, PushConstantsT>>
	>;
	
	using uniformsPackType = TypePack<WithShaderStage<shaderStage, uniformTs>...>;
	
	static constexpr std::string filename = filenameStringT::string;
};
template <typename filenameStringT, typename PushConstantsT, typename AttributesT, typename... uniformTs>
struct VertexShader
: public Shader<VK_SHADER_STAGE_VERTEX_BIT, filenameStringT, PushConstantsT, uniformTs...> {
	
	using AttributesType = AttributesT;
	
	AttributesT attributes;
};


// Shader program
// -----
template <typename pushConstantTP, typename UniformsT>
struct ShaderProgramBase {};

template <typename... pushConstantTs, typename UniformsT>
struct ShaderProgramBase<TypePack<pushConstantTs...>, UniformsT> {
	
	static constexpr uint32_t pushConstantCount = CountT<pushConstantTs...>();
	
	static constexpr std::array<VkPushConstantRange, pushConstantCount> pushConstantRanges = {(pushConstantTs::VkPushConstantRange, ...)};
	
	ShaderProgramBase(std::shared_ptr<Devices> _devices)
	: devices(_devices), uniforms(_devices) {
		// pipeline layout
		const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = UniformsT::descriptorSetCount, // Optional
			.pSetLayouts = uniforms.DescriptorSetLayouts().data(), // Optional
			.pushConstantRangeCount = pushConstantCount,
			.pPushConstantRanges = pushConstantCount == 0 ? nullptr : pushConstantRanges.data()
		};
		if(vkCreatePipelineLayout(devices->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS){
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}
	
	~ShaderProgramBase(){
		vkDestroyPipeline(devices->GetLogicalDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(devices->GetLogicalDevice(), layout, nullptr);
	}
	
	// ----- Methods to call after Init() -----
	// Bind the pipeline for subsequent render calls
	void CmdBind(VkCommandBuffer commandBuffer) const {
		vkCmdBindPipeline(commandBuffer, BindPoint(), pipeline);
	}
	// Set which descriptor sets are bound for subsequent render calls
	void CmdBindDescriptorSets(VkCommandBuffer commandBuffer, int flight, int first=0, int number=0, const std::vector<int> &dynamicOffsetNumbers=std::vector<int>()) const {
		
		if(number < 1){
			number = int(UniformsT::descriptorSetCount) - first;
		}
		if(!dynamicOffsetNumbers.empty()){
			std::vector<uint32_t> theseDynamicOffsets {};
			int indexOfDynamic = 0;
			for(int i=first; i<first + number; ++i){
				if(descriptorSets[i]->GetUBODynamic()){
					theseDynamicOffsets.push_back((uint32_t)(dynamicOffsetNumbers[indexOfDynamic] * descriptorSets[i]->GetUBODynamicAlignment().value()));
				}
			}
			assert(theseDynamicOffsets.size() == dynamicOffsetNumbers.size());
			vkCmdBindDescriptorSets(commandBuffer, BindPoint(), layout, first, number, uniforms.DescriptorSetsStart(flight), uint32_t(dynamicOffsetNumbers.size()), theseDynamicOffsets.data());
		} else {
			vkCmdBindDescriptorSets(commandBuffer, BindPoint(), layout, first, number, &uniforms.DescriptorSetsStart(flight), 0, nullptr);
		}
	}
	void UpdateDescriptorSets(uint32_t first=0); // have to do this every time any elements of any descriptors are changed, e.g. when an image view is re-created upon window resize
	// Set push constant data
	template <uint32_t index>
	void CmdPushConstants(VkCommandBuffer commandBuffer, IndexT<index, pushConstantTs...>::type *data){
		using pushConstantT = IndexT<index, pushConstantTs...>::type;
		vkCmdPushConstants(commandBuffer,
						   layout,
						   pushConstantT::stageFlagsValue,
						   pushConstantT::offsetValue,
						   sizeof(pushConstantT::type),
						   data);
	}
	
	// Get the handle of a descriptor set
	template <uint32_t index>
	UniformsT::iDescriptorSet<index> &DS(){ return uniforms.DescriptorSet<index>(); }
	
protected:
	std::shared_ptr<Devices> devices;
	
	UniformsT uniforms;
	
	VkPipelineLayout layout;
	
	VkPipeline pipeline;
	
	virtual VkPipelineBindPoint BindPoint() const = 0;
};

struct GraphicsPipelineBlueprint {
	VkPrimitiveTopology primitiveTopology;
	VkPipelineRasterizationStateCreateInfo rasterisationStateCI;
	VkPipelineMultisampleStateCreateInfo multisampleStateCI;
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI;
	VkPipelineColorBlendStateCreateInfo colourBlendStateCI;
	VkPipelineDynamicStateCreateInfo dynamicStateCI;
	VkRenderPass renderPassHandle;
};

template <typename vertexShaderT, typename fragmentShaderT>
struct GraphicsShaderProgram : public ShaderProgramBase<
	Merge_t<ConcatenatedPack_t<vertexShaderT::pushConstantsPackType, fragmentShaderT::pushConstantsPackType>>,
	Uniforms<Merge_t<ConcatenatedPack_t<vertexShaderT::uniformsPackType, fragmentShaderT::uniformsPackType>>>
> {
	GraphicsShaderProgram(std::shared_ptr<Devices> _devices, const GraphicsPipelineBlueprint &blueprint)
	: ShaderProgramBase(_devices) {
		// ----- Input assembly info -----
		const VkPipelineInputAssemblyStateCreateInfo inputAssembly {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = blueprint.primitiveTopology,
			.primitiveRestartEnable = VK_FALSE
		};
		// ----- Viewport state -----
		const VkPipelineViewportStateCreateInfo viewportState {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1
		};
		// Shader stages
		const VkPipelineShaderStageCreateInfo stageCIs[2] = {
			{// vertex shader
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.pName = "main",
				.module = devices->CreateShaderModule(vertexShaderT::filename),
				// this is for constants to use in the shader:
				.pSpecializationInfo = nullptr
			},
			{// fragment shader
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.pName = "main",
				.module = devices->CreateShaderModule(fragmentShaderT::filename),
				// this is for constants to use in the shader:
				.pSpecializationInfo = nullptr
			}
		};
		// Pipeline
		const VkGraphicsPipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = stageCIs.data(),
			.pVertexInputState = &vertexShaderT::AttributesType::pipelineVertexInputStateCI,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &blueprint.rasterisationStateCI,
			.pMultisampleState = &blueprint.multisampleStateCI,
			.pDepthStencilState = &blueprint.depthStencilStateCI, // Optional
			.pColorBlendState = &blueprint.colourBlendStateCI,
			.pDynamicState = &blueprint.dynamicStateCI,
			.layout = layout,
			.renderPass = blueprint.renderPassHandle,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE, // Optional
			.basePipelineIndex = -1 // Optional
		};
		if(vkCreateGraphicsPipelines(vulkan.devices->logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS){
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}
	~GraphicsShaderProgram(){
//		vkDestroyShaderModule(devices->GetLogicalDevice(), fragShaderModule, nullptr);
//		vkDestroyShaderModule(devices->GetLogicalDevice(), vertShaderModule, nullptr);
	}
	
protected:
	VkPipelineBindPoint BindPoint() const override { return VK_PIPELINE_BIND_POINT_GRAPHICS; }
};

template <typename computeShaderT>
struct ComputeShaderProgram : public ShaderProgramBase<computeShaderT::pushConstantsPackType> {
	ComputeShaderProgram(std::shared_ptr<Devices> _devices)
	: ShaderProgramBase(_devices) {
		
		// Shader stage
		const VkPipelineShaderStageCreateInfo stageCI = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.pName = "main",
			.module = devices->CreateShaderModule(computeShaderT::filename),
			// this is for constants to use in the shader:
			.pSpecializationInfo = nullptr
		};
		// Pipeline
		VkComputePipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = stageCI,
			.layout = layout
		};
		if(vkCreateComputePipelines(devices->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS){
			throw std::runtime_error("Failed to create compute pipeline!");
		}
	}
	~ComputeShaderProgram(){
//		vkDestroyShaderModule(devices->GetLogicalDevice(), shaderModule, nullptr);
	}
	
protected:
	VkPipelineBindPoint BindPoint() const override { return VK_PIPELINE_BIND_POINT_COMPUTE; }
};

} // namespace EVK
