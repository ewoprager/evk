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
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags, uint32_t count=1>
struct TextureImages : public UniformBase<set, binding, stageFlags> {
	using descriptorType = TextureImagesDescriptor<binding, stageFlags, count>;
};
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags, uint32_t count=1>
struct TextureSamplers : public UniformBase<set, binding, stageFlags> {
	using descriptorType = TextureSamplersDescriptor<binding, stageFlags, count>;
};
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags, uint32_t count=1>
struct CombinedImageSamplers : public UniformBase<set, binding, stageFlags> {
	using descriptorType = CombinedImageSamplersDescriptor<binding, stageFlags, count>;
};
template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags, uint32_t count=1>
struct StorageImages : public UniformBase<set, binding, stageFlags> {
	using descriptorType = StorageImagesDescriptor<binding, stageFlags, count>;
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
	
	DescriptorSet(std::shared_ptr<Devices> _devices, const DescriptorSetBlueprint &blueprint)
	: devices(_devices) {}
	~DescriptorSet(){}
	
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
	
	void Update(const std::function<VkDescriptorSet(uint32_t) &dstSetFromFlight){
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
						descriptorWrites.push_back(maybeDescriptorWrite);
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
	
	std::tuple<descriptorTs...> descriptors;
	
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
template <typename... uniformTs, template <uint32_t...> std::integer_sequence indexSequence, uint32_t... indices> struct UniformsImpl<TypePack<uniformTs...>, indexSequence<uint32_t, indices...>> {
	
	static_assert(UniformUnique<uniformTs...>(), "No two uniforms should have both matching set and binding.");
	
	static_assert((AllSmallerSetsExistInUniforms<uniformTs::setValue, uniformTs...>() && ...), "The union of descriptor set indices for a shader should be consecutive and contain 0.");
	
	template <uint32_t set> requires (DescriptorSetExistsInUniforms<set, uniformTs...>())
	using iDescriptorSet = DescriptorSet<FilteredDescriptorPack<set, uniformTs...>>;
	
	static constexpr uint32_t descriptorSetCount = DescriptorSetCount<uniformTs...>();
	
	template <uint32_t index>
	iDescriptorSet<index> &DescriptorSet(){ return std::get<index>(descriptorSets); }
	
	UniformsImpl(){
		
	}
	
	void UpdateDescriptorSets(){
		[&]<uint32_t... indices>(std::index_sequence<indices...>){
			(std::get<indices>(descriptorSets).Update([this](uint32_t flight) -> VkDescriptorSet {
				return descriptorSetsFlying[descriptorSetCount * flight + indices];
			}) , ...);
		}(std::make_integer_sequence<uint32_t, descriptorSetCount>{});
	}
	
private:
	std::tuple<iDescriptorSet<indices>...> descriptorSets;
	
	std::array<VkDescriptorSetLayout, descriptorSetCount> descriptorSetLayouts;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT * descriptorSetCount> descriptorSetsFlying;
	VkDescriptorPool descriptorPool;
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
