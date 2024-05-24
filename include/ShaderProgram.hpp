#pragma once

#include "Static.hpp"

#include "Descriptors/UBO.hpp"
#include "Descriptors/SBO.hpp"
#include "Descriptors/TextureImages.hpp"
#include "Descriptors/TextureSamplers.hpp"
#include "Descriptors/CombinedImageSampelers.hpp"
#include "Descriptors/StorageImages.hpp"

namespace EVK {

// Push constants
// -----
struct NoPushConstants {};
template <VkShaderStageFlags stageFlags, size_t offset, typename T> struct PushConstants {
	static constexpr VkShaderStageFlags stageFlagsValue = stageFlags;
	static constexpr uint32_t offsetValue = offset;
	using type = T;
};

// Uniform
// -----
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags> struct UniformBase {
	static constexpr uint32_t setValue = set;
	static constexpr uint32_t bindingValue = binding;
	
	using descriptorType = DescriptorBase<binding, stageFlags>;
};

template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags, typename T>
struct UBO : public UniformBase<set, binding, stageFlags> {
	using descriptorType = UBODescriptor<binding, stageFlags>;
};
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags, typename T>
struct SBO : public UniformBase<set, binding, stageFlags> {
	using descriptorType = SBODescriptor<binding, stageFlags>;
};
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags>
struct TextureImages : public UniformBase<set, binding, stageFlags> {
	using descriptorType = TextureImagesDescriptor<binding, stageFlags>;
};
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags>
struct TextureSamplers : public UniformBase<set, binding, stageFlags> {
	using descriptorType = TextureSamplersDescriptor<binding, stageFlags>;
};
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags>
struct CombinedImageSamplers : public UniformBase<set, binding, stageFlags> {
	using descriptorType = CombinedImageSamplersDescriptor<binding, stageFlags>;
};
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags>
struct StorageImages : public UniformBase<set, binding, stageFlags> {
	using descriptorType = StorageImagesDescriptor<binding, stageFlags>;
};

template <typename uniformT, typename... uniformTs> consteval bool UniformContains(){
	return ((uniformT::UniformBase::setValue == uniformTs::UniformBase::setValue && uniformT::UniformBase::bindingValue == uniformTs::UniformBase::bindingValue) || ...);
}
template <typename uniformT> consteval bool UniformUnique(){ return true; }
template <typename uniformT1, typename uniformT2, typename... uniformTs> consteval bool UniformUnique(){
	return !UniformContains<uniformT1, uniformT2, uniformTs...>() && UniformUnique<uniformT2, uniformTs...>();
}

template <unsigned set, typename uniformT>
using FilteredDescriptorPackOne = std::conditional_t<uniformT::setValue == set, TypePack<typename uniformT::descriptorType>, TypePack<>>;

template <unsigned set, typename... uniformTs>
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
	if constexpr (set == 0)
		return DescriptorSetExistsInUniforms<0, uniformTs...>();
	else
		return DescriptorSetExistsInUniforms<set, uniformTs..>() && AllSmallerSetsExistInUniforms<set - 1, uniformTs..>();
}


// Attributes
// -----
template <VkVertexInputBindingDescription... bindingDescriptions> struct BindingDescriptionPack {};
template <VkVertexInputAttributeDescription... attributeDescriptions> struct AttributeDescriptionPack {};
template <typename attributeTP, typename bindingDescriptionP, typename attributeDescriptionP> struct Attributes {};
template <typename... attributeTs, VkVertexInputBindingDescription... bindingDescriptions, VkVertexInputAttributeDescription... attributeDescriptions>
struct Attributes<TypePack<attributeTs...>, BindingDescriptionPack<bindingDescriptions...>, AttributeDescriptionPack<attributeDescriptions...>> {
	
	static consteval uint32_t TotalSize(){ return (sizeof(attributeTs) + ...); }
	
	static_assert((bindingDescriptions.stride + ...) == TotalSize(), "Vertex input binding description strides should sum to the total size of all the attributes.");
	
	static_assert(Unique<bindingDescriptions.binding...>(), "Vertex input binding description bindings should be unique.");
	
	static_assert(Unique<attributeDescriptions.location...>(), "Vertex input attribute description locations should be unique.");
	
	template <typename T> void BindVertexBuffer(){
		// check binding
		// ...
	}
};


// Descriptor set
// -----
template <typename descriptorTP> struct DescriptorSet {};
template <typename... descriptorTs>
struct DescriptorSet<TypePack<descriptorTs...>> {
	static constexpr uint32_t descriptorCount = CountT<descriptorTs...>();
	
	template <uint32_t index> using iDescriptor = IndexT<index, descriptorTs...>;
	
	DescriptorSet(Pipeline &_pipeline, int _index, const DescriptorSetBlueprint &blueprint);
	~DescriptorSet(){}
	
	// For initialisation
	void InitLayouts();
	void Update();
	
	size_t GetDescriptorCount() const { return descriptors.size(); }
	std::shared_ptr<Descriptor> GetDescriptor(int index) const { return descriptors[index]; }
	bool GetUBODynamic() const { return uboDynamicAlignment.has_value(); }
	const std::optional<VkDeviceSize> &GetUBODynamicAlignment() const { return uboDynamicAlignment; }
	
private:
	Pipeline &pipeline;
	int index;
	
	std::tuple<descriptorTs...> descriptors;
	
	// ubo info
	std::optional<VkDeviceSize> uboDynamicAlignment;
	
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
template <typename... uniformTs, template <uint32_t...> std::integer_sequence indexSequence, uint32_t... indices> struct UniformsImpl<TypePack<uniformTs...>, indexSequence<uint32_t, indices...>> {
	
	static_assert(UniformUnique<uniformTs...>(), "No two uniforms should have both matching set and binding.");
	
	static_assert((AllSmallerSetsExistInUniforms<uniformTs::setValue, uniformTs...>() && ...), "The union of descriptor set indices for a shader should be consecutive and contain 0.");
	
	template <uint32_t set> requires (DescriptorSetExistsInUniforms<set, uniformTs...>())
	using iDescriptorSet = DescriptorSet<FilteredDescriptorPack<set, uniformTs...>>;
	
	static constexpr uint32_t descriptorSetCount = DescriptorSetCount<uniformTs...>();
	
	template <uint32_t index>
	iDescriptorSet<index> &DescriptorSet(){ return std::get<index>(descriptorSets); }
	
private:
	std::tuple<iDescriptorSet<indices>...> descriptorSets;
};

template <typename... uniformTs> using Uniforms = UniformsImpl<TypePack<uniformTs...>, std::make_integer_sequence<uint32_t, DescriptorSetCount<uniformTs...>()>{}>;


// Shader
// -----
template <typename PushConstantsT, typename uniformTs...> struct Shader {
	using pushConstantsPackType = std::conditional_t<std::same_as<PushConstantsT, NoPushConstants>, TypePack<>, TypePack<PushConstantsT>>;
	using uniformsPackType = TypePack<uniformTs...>;
};
template <typename PushConstantsT, typename UniformsT, typename AttributesT>
struct VertexShader : public Shader<PushConstantsT, UniformsT> {
	AttributesT attributes;
};


// Shader program
// -----
template <typename pushConstantTP, typename UniformsT>
struct ShaderProgramBase {};

template <typename... pushConstantTs, typename UniformsT>
struct ShaderProgramBase<TypePack<pushConstantTs...>, UniformsT> {
	ShaderProgramBase(Interface &_vulkan, const PipelineBlueprint &blueprint);
	
	~ShaderProgramBase(){
		vkDestroyDescriptorPool(vulkan.devices.logicalDevice, descriptorPool, nullptr);
		vkDestroyPipeline(vulkan.devices.logicalDevice, pipeline, nullptr);
		vkDestroyPipelineLayout(vulkan.devices.logicalDevice, layout, nullptr);
		for(VkDescriptorSetLayout &dsl : descriptorSetLayouts){
			vkDestroyDescriptorSetLayout(vulkan.devices.logicalDevice, dsl, nullptr);
		}
	}
	
	// ----- Methods to call after Init() -----
	// Bind the pipeline for subsequent render calls
	virtual void Bind() = 0;
	// Set which descriptor sets are bound for subsequent render calls
	virtual void BindDescriptorSets(int first=0, int number=0, const std::vector<int> &dynamicOffsetNumbers=std::vector<int>()) = 0;
	void UpdateDescriptorSets(uint32_t first=0); // have to do this every time any elements of any descriptors are changed, e.g. when an image view is re-created upon window resize
	// Set push constant data
	template <uint32_t index>
	void CmdPushConstants(IndexT<index, pushConstantTs...>::type *data){
		using pushConstantT = IndexT<index, pushConstantTs...>;
		vkCmdPushConstants(vulkan.commandBuffersFlying[vulkan.currentFrame],
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
	Interface &vulkan;
	
	UniformsT uniforms;
	
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkDescriptorSet> descriptorSetsFlying;
	VkDescriptorPool descriptorPool;
	
	std::tuple<pushConstantTs...> pushConstants;
	
	VkPipelineLayout layout;
	VkPipeline pipeline;
};

template <typename vertexShaderT, typename fragmentShaderT>
struct GraphicsShaderProgram : public ShaderProgramBase<
	ConcatenatedPack<vertexShaderT::pushConstantsPackType, fragmentShaderT::pushConstantsPackType>,
	Uniforms<ConcatenatedPack<vertexShaderT::uniformsPackType, fragmentShaderT::uniformsPackType>>
> {
	GraphicsPipeline(Interface &_vulkan, const GraphicsPipelineBlueprint &blueprint);
	~GraphicsPipeline(){
		vkDestroyShaderModule(vulkan.devices.logicalDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(vulkan.devices.logicalDevice, vertShaderModule, nullptr);
	}
	
	void Bind() override;
	void BindDescriptorSets(int first=0, int number=0, const std::vector<int> &dynamicOffsetNumbers=std::vector<int>()) override;
	
private:
	vertexShaderT vertexShader;
	fragmentShaderT fragmentShader;
	
//			VkShaderModule vertShaderModule; ?
//			VkShaderModule fragShaderModule; ?
};

template <typename computeShaderT>
struct ComputeShaderProgram : public ShaderProgramBase<computeShaderT::pushConstantsPackType> {
	ComputePipeline(Interface &_vulkan, const ComputePipelineBlueprint &blueprint);
	~ComputePipeline(){
		vkDestroyShaderModule(vulkan.devices.logicalDevice, shaderModule, nullptr);
	}
	
	void Bind() override;
	void BindDescriptorSets(int first=0, int number=0, const std::vector<int> &dynamicOffsetNumbers=std::vector<int>()) override;
	
private:
	computeShaderT computeShader;
	
//		VkShaderModule shaderModule; ?
};

} // namespace EVK
