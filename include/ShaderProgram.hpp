#pragma once

#include "Static.hpp"

#include "Devices.hpp"

#include "UBODescriptor.hpp"
#include "SBODescriptor.hpp"
#include "TextureImagesDescriptor.hpp"
#include "TextureSamplersDescriptor.hpp"
#include "CombinedImageSamplersDescriptor.hpp"
#include "StorageImagesDescriptor.hpp"

namespace EVK {

// Push constants
// -----
struct NoPushConstants {};
template <size_t offset, typename T> struct PushConstants {
	static constexpr uint32_t offsetValue = offset;
	using type = T;
};

template <typename T>
concept notNoPushConstants_c = requires {
	{T::offsetValue} -> std::same_as<uint32_t>;
	typename T::type;
};

template <typename T>
concept pushConstants_c = requires {
	std::same_as<T, NoPushConstants> || notNoPushConstants_c<T>;
};

template <VkShaderStageFlags stageFlags, typename T> struct WithShaderStage {
	static constexpr VkShaderStageFlags stageFlagsValue = stageFlags;
	using type = T;
//	static constexpr VkPushConstantRange rangeValue = {};
};
template <VkShaderStageFlags stageFlags, size_t offset, typename T>
struct WithShaderStage<stageFlags, PushConstants<offset, T>> {
	static constexpr VkShaderStageFlags stageFlagsValue = stageFlags;
	using type = PushConstants<offset, T>;
	static constexpr VkPushConstantRange rangeValue = {stageFlags, offset, sizeof(T)};
};

template <typename T>
concept withShaderStage_c = requires {
	{T::stageFlagsValue} -> std::same_as<VkShaderStageFlags>;
	typename T::type;
};

template <typename T>
concept pushConstantWithShaderStage_c = requires {
	withShaderStage_c<T>;
	pushConstants_c<typename T::type>;
};

// merge one WithShaderStage into a pack of pre-merged WithShaderStages
template <typename checkedTP, typename withShaderStageT, typename notCheckedTP> struct OneWithShaderStageMergedIntoMerged;
template <typename... checkedTs, typename withShaderStageT> struct OneWithShaderStageMergedIntoMerged<TypePack<checkedTs...>, withShaderStageT, TypePack<>> {
	using packType = TypePack<checkedTs..., withShaderStageT>;
};
template <typename... checkedTs, typename withShaderStageT, typename notCheckedT>
requires (withShaderStage_c<withShaderStageT>)
struct OneWithShaderStageMergedIntoMerged<TypePack<checkedTs...>, withShaderStageT, TypePack<notCheckedT>> {
	using packType = std::conditional_t<
	std::same_as<typename withShaderStageT::type, typename notCheckedT::type>,
	TypePack<checkedTs..., WithShaderStage<withShaderStageT::stageFlagsValue | notCheckedT::stageFlagsValue, typename withShaderStageT::type>>,
	TypePack<checkedTs..., withShaderStageT, notCheckedT>
	>;
};
template <typename... checkedTs, typename withShaderStageT, typename notCheckedT, typename... rest> struct OneWithShaderStageMergedIntoMerged<TypePack<checkedTs...>, withShaderStageT, TypePack<notCheckedT, rest...>> {
	using packType = std::conditional_t<
	std::same_as<typename withShaderStageT::type, typename notCheckedT::type>,
	TypePack<checkedTs..., WithShaderStage<withShaderStageT::stageFlagsValue | notCheckedT::stageFlagsValue, typename withShaderStageT::type>, rest...>,
	typename OneWithShaderStageMergedIntoMerged<TypePack<checkedTs..., notCheckedT>, withShaderStageT, TypePack<rest...>>::packType
	>;
};
template <typename T>
concept oneWithShaderStageMergedIntoMerged_c = requires {
	typename T::packType;
};
template <typename withShaderStageT, typename mergedTP>
requires (oneWithShaderStageMergedIntoMerged_c<OneWithShaderStageMergedIntoMerged<TypePack<>, withShaderStageT, mergedTP>>)
using oneWithShaderStageMergedIntoMerged_t = typename OneWithShaderStageMergedIntoMerged<TypePack<>, withShaderStageT, mergedTP>::packType;

// merge all of a pack of WithShaderStages together
template <typename mergedTP, typename unmergedTP> struct WithShaderStagesMerged;
template <typename... mergedTs> struct WithShaderStagesMerged<TypePack<mergedTs...>, TypePack<>> {
	using packType = TypePack<mergedTs...>;
};
template <typename... mergedTs, typename unmergedT> struct WithShaderStagesMerged<TypePack<mergedTs...>, TypePack<unmergedT>> {
	using packType = oneWithShaderStageMergedIntoMerged_t<unmergedT, TypePack<mergedTs...>>;
};
template <typename... mergedTs, typename unmergedT, typename U, typename... rest> struct WithShaderStagesMerged<TypePack<mergedTs...>, TypePack<unmergedT, U, rest...>> {
	using packType = typename WithShaderStagesMerged<oneWithShaderStageMergedIntoMerged_t<unmergedT, TypePack<mergedTs...>>, TypePack<U, rest...>>::packType;
};
template <typename withShaderStageTP>
using withShaderStagesMerged_t = typename WithShaderStagesMerged<TypePack<>, withShaderStageTP>::packType;


// Uniform
// -----
template <uint32_t set, uint32_t binding> struct UniformBase {
	static constexpr uint32_t setValue = set;
	static constexpr uint32_t bindingValue = binding;
};

template <uint32_t set, uint32_t binding, typename T, bool dynamic=false>
struct UBOUniform : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptor_t = UBODescriptor<binding, stageFlags, T, dynamic>;
};
template <uint32_t set, uint32_t binding>
struct SBOUniform : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptor_t = SBODescriptor<binding, stageFlags>;
};
template <uint32_t set, uint32_t binding, uint32_t count=1>
struct TextureImagesUniform : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptor_t = TextureImagesDescriptor<binding, stageFlags, count>;
};
template <uint32_t set, uint32_t binding, uint32_t count=1>
struct TextureSamplersUniform : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptor_t = TextureSamplersDescriptor<binding, stageFlags, count>;
};
template <uint32_t set, uint32_t binding, uint32_t count=1>
struct CombinedImageSamplersUniform : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptor_t = CombinedImageSamplersDescriptor<binding, stageFlags, count>;
};
template <uint32_t set, uint32_t binding, uint32_t count=1>
struct StorageImagesUniform : public UniformBase<set, binding> {
	template <VkShaderStageFlags stageFlags>
	using descriptor_t = StorageImagesDescriptor<binding, stageFlags, count>;
};

template <typename T>
concept descriptor_c = requires (T val) {
	{T::bindingValue} -> std::same_as<const uint32_t &>;
	{T::stageFlagsValue} -> std::same_as<const uint32_t &>;
	{T::layoutBinding} -> std::same_as<const VkDescriptorSetLayoutBinding&>;
	{T::poolSize} -> std::same_as<const VkDescriptorPoolSize &>;
};

struct NoDescriptorInDescriptorSetWithThatBinding{};

template <uint32_t binding, typename... descriptor_ts> struct DescriptorByBinding;
template <uint32_t binding, typename descriptor_t>
struct DescriptorByBinding<binding, descriptor_t> {
	using type = std::conditional_t<
	descriptor_t::bindingValue == binding,
	descriptor_t,
	NoDescriptorInDescriptorSetWithThatBinding
	>;
};
template <uint32_t binding, typename descriptor_t, typename descriptor_u, typename... descriptor_ts>
struct DescriptorByBinding<binding, descriptor_t, descriptor_u, descriptor_ts...> {
	using type = std::conditional_t<
	descriptor_t::bindingValue == binding,
	descriptor_t,
	typename DescriptorByBinding<binding, descriptor_u, descriptor_ts...>::type
	>;
};

template <uint32_t binding, typename... descriptor_ts>
requires ((descriptor_c<descriptor_ts> && ...))
using descriptorByBinding_t = typename DescriptorByBinding<binding, descriptor_ts...>::type;

template <typename T, VkShaderStageFlags stageFlags>
concept uniform_c = requires {
	typename T::template descriptor_t<stageFlags>;
	descriptor_c<typename T::template descriptor_t<stageFlags>>;
};

template <typename T>
concept uniformWithShaderStage_c = requires {
	T::stageFlagsValue;
	typename T::type;
	uniform_c<typename T::type, T::stageFlagsValue>;
};

template <typename uniformWithShaderStage_t>
requires (uniformWithShaderStage_c<uniformWithShaderStage_t>)
using descriptor_t = uniformWithShaderStage_t::type::template descriptor_t<uniformWithShaderStage_t::stageFlagsValue>;

template <typename uniformWithShaderStage_t, typename... uniformWithShaderStage_ts>
requires (uniformWithShaderStage_c<uniformWithShaderStage_t> && (uniformWithShaderStage_c<uniformWithShaderStage_ts> && ...))
consteval bool UniformIn(){
	return ((uniformWithShaderStage_t::type::setValue == uniformWithShaderStage_ts::type::setValue && uniformWithShaderStage_t::type::bindingValue == uniformWithShaderStage_ts::type::bindingValue) || ...);
}

template <typename... uniformWithShaderStage_ts>
struct UniformsUniqueImpl;
template <>
struct UniformsUniqueImpl<> {
	static constexpr bool value = true;
};
template <typename uniformWithShaderStage_t>
requires (uniformWithShaderStage_c<uniformWithShaderStage_t>)
struct UniformsUniqueImpl<uniformWithShaderStage_t>{
	static constexpr bool value = true;
};
template <typename uniformT1, typename uniformT2, typename... uniformWithShaderStage_ts>
requires (uniformWithShaderStage_c<uniformT1> && uniformWithShaderStage_c<uniformT2> && (uniformWithShaderStage_c<uniformWithShaderStage_ts> && ...))
struct UniformsUniqueImpl<uniformT1, uniformT2, uniformWithShaderStage_ts...> {
	static constexpr bool value = !UniformIn<uniformT1, uniformT2, uniformWithShaderStage_ts...>() && UniformsUniqueImpl<uniformT2, uniformWithShaderStage_ts...>::value;
};
template <typename... uniformTs>
consteval bool UniformsUnique(){ return UniformsUniqueImpl<uniformTs...>::value; }

template <uint32_t set, typename uniformWithShaderStage_t>
requires (uniformWithShaderStage_c<uniformWithShaderStage_t>)
using filteredDescriptorPackOne_t = std::conditional_t<
	uniformWithShaderStage_t::type::setValue == set,
	TypePack<descriptor_t<uniformWithShaderStage_t>>,
	TypePack<>
	>;

template <uint32_t set, typename... uniformWithShaderStage_ts>
requires ((uniformWithShaderStage_c<uniformWithShaderStage_ts> && ...))
using filteredDescriptorPack_t = concatenatedPack_t<filteredDescriptorPackOne_t<set, uniformWithShaderStage_ts>...>;

template <uint32_t set, typename... uniformWithShaderStage_ts>
requires ((uniformWithShaderStage_c<uniformWithShaderStage_ts> && ...))
static consteval bool DescriptorSetExistsInUniforms(){
	return !filteredDescriptorPack_t<set, uniformWithShaderStage_ts...>::empty;
}

template <uint32_t set, typename... uniformWithShaderStage_ts>
requires ((uniformWithShaderStage_c<uniformWithShaderStage_ts> && ...))
static consteval uint32_t DescriptorSetCountImpl(){
	if constexpr (DescriptorSetExistsInUniforms<set, uniformWithShaderStage_ts...>()){
		return DescriptorSetCountImpl<set + 1, uniformWithShaderStage_ts...>();
	} else {
		return set;
	}
}

template <typename... uniformWithShaderStage_ts>
requires ((uniformWithShaderStage_c<uniformWithShaderStage_ts> && ...))
static constexpr uint32_t DescriptorSetCount(){
	return DescriptorSetCountImpl<0, uniformWithShaderStage_ts...>();
}

template <uint32_t set, typename... uniformWithShaderStage_ts>
requires ((uniformWithShaderStage_c<uniformWithShaderStage_ts> && ...))
static consteval bool AllSmallerSetsExistInUniforms(){
	if constexpr (set == 0){
		return DescriptorSetExistsInUniforms<0, uniformWithShaderStage_ts...>();
	} else {
		return DescriptorSetExistsInUniforms<set, uniformWithShaderStage_ts...>() && AllSmallerSetsExistInUniforms<set - 1, uniformWithShaderStage_ts...>();
	}
}


// Attributes
// -----
template <VkVertexInputBindingDescription... bindingDescriptions> struct BindingDescriptionPack {};
template <VkVertexInputAttributeDescription... attributeDescriptions> struct AttributeDescriptionPack {};

struct PipelineVertexInputStateCreateInfoSafe {
	std::vector<VkVertexInputBindingDescription> binding;
	std::vector<VkVertexInputAttributeDescription> attribute;
};

template <typename bindingDescriptionP, typename attributeDescriptionP> struct Attributes;
template <VkVertexInputBindingDescription... bindingDescriptions, VkVertexInputAttributeDescription... attributeDescriptions>
struct Attributes<BindingDescriptionPack<bindingDescriptions...>, AttributeDescriptionPack<attributeDescriptions...>> {
	
	static constexpr uint32_t bindingDescriptionCount = sizeof...(bindingDescriptions);
	
	static constexpr uint32_t attributeDescriptionCount = sizeof...(attributeDescriptions);
	
	static_assert(Unique<bindingDescriptions.binding...>(), "Vertex input binding description bindings should be unique.");
	
	static_assert(Unique<attributeDescriptions.location...>(), "Vertex input attribute description locations should be unique.");
	
//	static constexpr VkVertexInputBindingDescription bindingDescriptionsValue[bindingDescriptionCount] = {(bindingDescriptions, ...)};
//	
//	static constexpr VkVertexInputAttributeDescription attributeDescriptionsValue[attributeDescriptionCount] = {(attributeDescriptions, ...)};
	
//	static constexpr VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCI = {
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.vertexBindingDescriptionCount = bindingDescriptionCount,
//		.pVertexBindingDescriptions = (VkVertexInputBindingDescription[bindingDescriptionCount]){
//			(bindingDescriptions, ...)
//		},
//		.vertexAttributeDescriptionCount = attributeDescriptionCount,
//		.pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[attributeDescriptionCount]){
//			(attributeDescriptions, ...)
//		}
//	};
	
	static PipelineVertexInputStateCreateInfoSafe PipelineVertexInputStateCI(){
		size_t i;
		PipelineVertexInputStateCreateInfoSafe ret {};
		
		ret.binding.resize(bindingDescriptionCount);
		i = 0;
		(void(ret.binding[i++] = bindingDescriptions), ...);
		
		ret.attribute.resize(attributeDescriptionCount);
		i = 0;
		(void(ret.attribute[i++] = attributeDescriptions), ...);
		
		return ret;
	}
};

// Descriptor set
// -----
template <typename descriptor_tp, typename indexSequence_t> struct DescriptorSetImpl;
template <typename... descriptor_ts, uint32_t... indices>
struct DescriptorSetImpl<TypePack<descriptor_ts...>, std::integer_sequence<uint32_t, indices...>> {
	
	static constexpr uint32_t descriptorCount = sizeof...(descriptor_ts);
	
	template <uint32_t index> using descriptor_t = descriptorByBinding_t<index, descriptor_ts...>;
	
	// needs to have default constructor jsut so that UniformsImpl can initialise in body of its constructor
	DescriptorSetImpl(){}
	
	explicit DescriptorSetImpl(std::shared_ptr<Devices> _devices)
	: devices(_devices) {}
	
	VkDescriptorSetLayout CreateLayout() const {
		std::array<VkDescriptorSetLayoutBinding, descriptorCount> layoutBindings {};
		size_t i = 0;
		(void(layoutBindings[i++] = descriptor_t<indices>::layoutBinding), ...);
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
	
	bool Update(const std::function<const VkDescriptorSet &(uint32_t)> &dstSetFromFlight){
		VkDescriptorBufferInfo bufferInfos[32];
		VkDescriptorImageInfo imageInfos[256];
		int bufferInfoCounter, imageInfoCounter;
		
		// configuring descriptors in descriptor sets
		for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++){
			std::vector<VkWriteDescriptorSet> descriptorWrites{};
			
			bufferInfoCounter = 0;
			imageInfoCounter = 0;
			
			if(!([&]() -> bool {
				const std::optional<VkWriteDescriptorSet> maybeDescriptorWrite = std::get<indices>(descriptors).DescriptorWrite(dstSetFromFlight(i), imageInfos, imageInfoCounter, bufferInfos, bufferInfoCounter, i);
				if(!maybeDescriptorWrite){
					return false;
				}
				descriptorWrites.push_back(maybeDescriptorWrite.value());
				return true;
			}() && ...)){
				return false;
			}
			vkUpdateDescriptorSets(devices->GetLogicalDevice(), uint32_t(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
		// telling descriptors that they are valid
		(void(std::get<indices>(descriptors).SetValid()), ...);
		
		return true;
	}
	
	bool CheckDescriptorsValid() const {
		return (std::get<indices>(descriptors).Valid() && ...);
	}
	
	template <uint32_t index>
	descriptor_t<index> &iDescriptor() {
		return std::get<index>(descriptors);
	}
	
	std::optional<VkDeviceSize> GetUBODynamicAlignment() const {
		std::optional<VkDeviceSize> ret {};
		([&](){
			if(std::optional<DynamicUBOInfo> mbdyn = std::get<indices>(descriptors).GetUBODynamic(); mbdyn.has_value()){
				if(ret.has_value()){
					throw std::runtime_error("A descriptor set can contain at most 1 uniform buffer object.");
				} else {
					ret = mbdyn->alignment;
				}
			}
		}(), ...);
		return ret;
	}
	
private:
	std::shared_ptr<Devices> devices;
	
	std::tuple<descriptor_t<indices>...> descriptors {};
	
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

template <typename descriptor_tp> struct DescriptorSetStruct;
template <typename... descriptor_ts> struct DescriptorSetStruct<TypePack<descriptor_ts...>> {
	using type = DescriptorSetImpl<TypePack<descriptor_ts...>, std::make_integer_sequence<uint32_t, sizeof...(descriptor_ts)>>;
};
template <typename descriptor_tp>
using DescriptorSet = typename DescriptorSetStruct<descriptor_tp>::type;


// Uniforms
// -----
template <typename uniformWithShaderStages_tp, typename indexSequence_t> class UniformsImpl;
// specialisation for some uniforms
template <typename... uniformWithShaderStage_ts, uint32_t... indices>
requires ((uniformWithShaderStage_c<uniformWithShaderStage_ts> && ...))
class UniformsImpl<TypePack<uniformWithShaderStage_ts...>, std::integer_sequence<uint32_t, indices...>> {
public:
	static_assert(UniformsUnique<uniformWithShaderStage_ts...>(), "No two uniforms should have both matching set and binding.");
	
	static_assert((AllSmallerSetsExistInUniforms<uniformWithShaderStage_ts::type::setValue, uniformWithShaderStage_ts...>() && ...), "The union of descriptor set indices for a shader should be consecutive and contain 0.");
	
	template <uint32_t set>
	requires (DescriptorSetExistsInUniforms<set, uniformWithShaderStage_ts...>())
	using descriptorSet_t = DescriptorSet<filteredDescriptorPack_t<set, uniformWithShaderStage_ts...>>;
	
	static constexpr uint32_t descriptorSetCount = DescriptorSetCount<uniformWithShaderStage_ts...>();
	
	static constexpr uint32_t uniformCount = sizeof...(uniformWithShaderStage_ts);
//	static_assert((uint32_t descriptorSet_t<indices>::descriptorCount + ...) == uniformCount, "!Inconsistency");
	
	static consteval std::array<VkDescriptorPoolSize, uniformCount> PoolSizes(){
		if constexpr (uniformCount == 0){
			return {{}};
		} else {
			std::array<VkDescriptorPoolSize, uniformCount> ret;
			size_t i = 0;
			(void(ret[i++] = descriptor_t<uniformWithShaderStage_ts>::poolSize), ...);
			return ret;
		}
	}
	
//	template <uint32_t index>
//	descriptorSet_t<index> &iDescriptorSet(){ return std::get<index>(descriptorSets); }
	
	explicit UniformsImpl(std::shared_ptr<Devices> _devices)
	: devices(_devices)
//	, descriptorSets(sizeof...(indices) == 0 ? {} : (descriptorSet_t<indices>(_devices), ...)) 
	{
		if constexpr (sizeof...(indices) == 0){
			descriptorSets = {};
		} else {
			(void(std::get<indices>(descriptorSets) = descriptorSet_t<indices>(_devices)), ...);
		}
		
		std::array<VkDescriptorPoolSize, uniformCount> poolSizes = PoolSizes();
		
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
		if(vkAllocateDescriptorSets(_devices->GetLogicalDevice(), &allocInfo, descriptorSetsFlying.data()) != VK_SUCCESS){
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
	}
	~UniformsImpl(){
		vkDestroyDescriptorPool(devices->GetLogicalDevice(), descriptorPool, nullptr);
		for(VkDescriptorSetLayout &dsl : descriptorSetLayouts){
			vkDestroyDescriptorSetLayout(devices->GetLogicalDevice(), dsl, nullptr);
		}
	}
	
	// have to do this every time any elements of any descriptors are changed, e.g. when an image view is re-created upon window resize
	template <uint32_t first, uint32_t number>
	requires (first + number <= descriptorSetCount)
	bool UpdateDescriptorSets(){
		if(CheckDescriptorSetsValid<first, number>()){
			return true;
		}
		return [&]<uint32_t... indexSubset>(std::integer_sequence<uint32_t, indexSubset...>) -> bool {
			return (std::get<indexSubset + first>(descriptorSets).Update([&](uint32_t flight) -> const VkDescriptorSet & {
				return descriptorSetsFlying[descriptorSetCount * flight + indexSubset + first];
			}) && ...);
		}(std::make_integer_sequence<uint32_t, number>{});
	}
	
	const std::array<VkDescriptorSetLayout, descriptorSetCount> &DescriptorSetLayouts() const {
		return descriptorSetLayouts;
	}
	
	const VkDescriptorSet *DescriptorSetsStart(uint32_t flight){
		return &descriptorSetsFlying[flight * descriptorSetCount];
	}
	
	template <uint32_t first, uint32_t number>
	requires (first + number <= descriptorSetCount)
	std::vector<uint32_t> GetDynamicOffsets(const std::vector<int> &dynamicOffsetNumbers) const {
		std::vector<uint32_t> ret {};
		int indexOfDynamic = 0;
		[&]<uint32_t... indexSubset>(std::integer_sequence<uint32_t, indexSubset...>){
			(void([&](){
				const std::optional<VkDeviceSize> dynamicAlignment = std::get<indexSubset + first>(descriptorSets).GetUBODynamicAlignment();
				if(dynamicAlignment){
					ret.push_back(uint32_t(dynamicOffsetNumbers[indexOfDynamic++] * dynamicAlignment.value()));
				}
			}), ...);
		}(std::make_integer_sequence<uint32_t, number>{});
//		assert(ret.size() == dynamicOffsetNumbers.size());
		return ret;
	}
	
	template <uint32_t index>
	descriptorSet_t<index> &iDescriptorSet(){
		return std::get<index>(descriptorSets);
	}
	
private:
	std::shared_ptr<Devices> devices;
	
	std::tuple<descriptorSet_t<indices>...> descriptorSets;
	
	std::array<VkDescriptorSetLayout, descriptorSetCount> descriptorSetLayouts {};
	std::array<VkDescriptorSet, descriptorSetCount * MAX_FRAMES_IN_FLIGHT> descriptorSetsFlying {};
	VkDescriptorPool descriptorPool;
	
	template <uint32_t first, uint32_t number>
	requires (first + number <= descriptorSetCount)
	bool CheckDescriptorSetsValid() const {
		return [&]<uint32_t... indexSubset>(std::integer_sequence<uint32_t, indexSubset...>) -> bool {
			return (std::get<indexSubset + first>(descriptorSets).CheckDescriptorsValid() && ...);
		}(std::make_integer_sequence<uint32_t, number>{});
	}
};

template <typename uniformWithShaderStage_tp> struct UniformsStruct;
template <typename... uniformWithShaderStage_ts> struct UniformsStruct<TypePack<uniformWithShaderStage_ts...>> {
	using type = UniformsImpl<TypePack<uniformWithShaderStage_ts...>, std::make_integer_sequence<uint32_t, DescriptorSetCount<uniformWithShaderStage_ts...>()>>;
};
template <typename uniformWithShaderStage_tp>
using Uniforms = typename UniformsStruct<uniformWithShaderStage_tp>::type;


// Shader
// -----
template <VkShaderStageFlags shaderStage, const char *filename, typename pushConstants_t, typename... uniform_ts>
requires ((uniform_c<uniform_ts, shaderStage> && ...) && pushConstants_c<pushConstants_t>)
struct Shader {
	using pushConstantWithShaderStage_tp = std::conditional_t<
	std::same_as<pushConstants_t, NoPushConstants>,
	TypePack<>,
	TypePack<WithShaderStage<shaderStage, pushConstants_t>>
	>;
	
	using uniformWithShaderStage_tp = TypePack<WithShaderStage<shaderStage, uniform_ts>...>;
	
	static constexpr std::string_view filenameValue = {filename};
};

template <typename T>
concept shader_c = requires {
	typename T::pushConstantWithShaderStage_tp;
	typename T::uniformWithShaderStage_tp;
	{T::filenameValue} -> std::same_as<const std::string_view &>;
};

template <const char *filename, typename pushConstants_t, typename attributes_t, typename... uniform_ts>
struct VertexShader
: public Shader<VK_SHADER_STAGE_VERTEX_BIT, filename, pushConstants_t, uniform_ts...> {
	
//	static constexpr const VkPipelineVertexInputStateCreateInfo *const pipelineVertexInputStateCI = &attributes_t::pipelineVertexInputStateCI;
	static PipelineVertexInputStateCreateInfoSafe PipelineVertexInputStateCI(){ return attributes_t::PipelineVertexInputStateCI(); }
};

template <typename T>
concept vertexShader_c = requires (T val) {
	shader_c<T>;
//	{T::pipelineVertexInputStateCI} -> std::same_as<const VkPipelineVertexInputStateCreateInfo *const &>;
	{T::PipelineVertexInputStateCI()} -> std::same_as<PipelineVertexInputStateCreateInfoSafe>;
};

// Pipeline
// -----
template <typename pushConstantWithShaderStages_tp> struct PushConstantManager;
template <typename... pushConstantWithShaderStage_ts>
requires ((pushConstantWithShaderStage_c<pushConstantWithShaderStage_ts> && ...))
struct PushConstantManager<TypePack<pushConstantWithShaderStage_ts...>> {
	
	static constexpr uint32_t pushConstantCount = sizeof...(pushConstantWithShaderStage_ts);
	
	static std::array<VkPushConstantRange, pushConstantCount> PushConstantRanges() {
		if constexpr (pushConstantCount == 0){
			return {};
		} else {
			std::array<VkPushConstantRange, pushConstantCount> ret {};
			size_t i = 0;
			(void(ret[i++] = pushConstantWithShaderStage_ts::rangeValue), ...);
			return ret;
		}
	}
	
	template <uint32_t index>
	using pushConstantWithShaderStage_t = index_t<index, pushConstantWithShaderStage_ts...>;
};

//template <typename vertexShader_t, typename fragmentShader_t>
//requires (vertexShader_c<vertexShader_t> && shader_c<fragmentShader_t>)
//using renderPipelineBase_t = PipelineBase<
//withShaderStagesMerged_t<concatenatedPack_t<typename vertexShader_t::pushConstantWithShaderStage_tp, typename fragmentShader_t::pushConstantWithShaderStage_tp>>,
//Uniforms<withShaderStagesMerged_t<concatenatedPack_t<typename vertexShader_t::uniformWithShaderStage_tp, typename fragmentShader_t::uniformWithShaderStage_tp>>>
//>;

//static constexpr VkPipelineVertexInputStateCreateInfo vertexCI = {
//	VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
//	nullptr,
//	NULL,
//	1,
//	(VkVertexInputBindingDescription[1]){
//		{
//			0, // binding
//			20, // stride
//			VK_VERTEX_INPUT_RATE_VERTEX // input rate
//		}
//	},
//	2,
//	(VkVertexInputAttributeDescription[2]){
//		VkVertexInputAttributeDescription{
//			1, 0, VK_FORMAT_R32G32B32_SFLOAT, 8
//		},
//		VkVertexInputAttributeDescription{
//			0, 0, VK_FORMAT_R32G32_SFLOAT, 0
//		}
//	}
//};

struct RenderPipelineBlueprint {
	VkPrimitiveTopology primitiveTopology;
	VkPipelineRasterizationStateCreateInfo *pRasterisationStateCI;
	VkPipelineMultisampleStateCreateInfo *pMultisampleStateCI;
	VkPipelineDepthStencilStateCreateInfo *pDepthStencilStateCI;
	VkPipelineColorBlendStateCreateInfo *pColourBlendStateCI;
	VkPipelineDynamicStateCreateInfo *pDynamicStateCI;
	VkRenderPass renderPassHandle;
};

template <typename vertexShader_t, typename fragmentShader_t>
requires (vertexShader_c<vertexShader_t> && shader_c<fragmentShader_t>)
class RenderPipeline {
public:
	using uniforms_t = Uniforms<withShaderStagesMerged_t<concatenatedPack_t<typename vertexShader_t::uniformWithShaderStage_tp, typename fragmentShader_t::uniformWithShaderStage_tp>>>;
	
	using pushConstantManager_t = PushConstantManager<withShaderStagesMerged_t<concatenatedPack_t<typename vertexShader_t::pushConstantWithShaderStage_tp, typename fragmentShader_t::pushConstantWithShaderStage_tp>>>;
	
	static constexpr uint32_t descriptorSetCount = uniforms_t::descriptorSetCount;
	
	RenderPipeline(std::shared_ptr<Devices> _devices, const RenderPipelineBlueprint *const pBlueprint)
	: devices(_devices), uniforms(_devices) {
		// pipeline layout
		std::array<VkPushConstantRange, pushConstantManager_t::pushConstantCount> pcrs = pushConstantManager_t::PushConstantRanges();
		const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = uniforms_t::descriptorSetCount, // Optional
			.pSetLayouts = uniforms.DescriptorSetLayouts().data(), // Optional
			.pushConstantRangeCount = pushConstantManager_t::pushConstantCount,
			.pPushConstantRanges = pushConstantManager_t::pushConstantCount == 0 ? nullptr : pcrs.data()
		};
		if(vkCreatePipelineLayout(_devices->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS){
			throw std::runtime_error("Failed to create pipeline layout!");
		}
		
		// ----- Input assembly info -----
		const VkPipelineInputAssemblyStateCreateInfo inputAssembly {
			 .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			 .topology = pBlueprint->primitiveTopology,
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
				 .module = devices->CreateShaderModule(vertexShader_t::filenameValue.data()),
				 // this is for constants to use in the shader:
				 .pSpecializationInfo = nullptr
			 },
			 {// fragment shader
				 .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				 .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				 .pName = "main",
				 .module = devices->CreateShaderModule(fragmentShader_t::filenameValue.data()),
				 // this is for constants to use in the shader:
				 .pSpecializationInfo = nullptr
			 }
		};
		
		const PipelineVertexInputStateCreateInfoSafe pviscis = vertexShader_t::PipelineVertexInputStateCI();
		const VkPipelineVertexInputStateCreateInfo pvisci = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			nullptr,
			NULL,
			uint32_t(pviscis.binding.size()),
			pviscis.binding.data(),
			uint32_t(pviscis.attribute.size()),
			pviscis.attribute.data()
		};
		
		// Pipeline
		const VkGraphicsPipelineCreateInfo pipelineInfo {
			 .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			 .stageCount = 2,
			 .pStages = stageCIs,
			 .pVertexInputState = &pvisci,//vertexShader_t::pipelineVertexInputStateCI,
			 .pInputAssemblyState = &inputAssembly,
			 .pViewportState = &viewportState,
			 .pRasterizationState = pBlueprint->pRasterisationStateCI,
			 .pMultisampleState = pBlueprint->pMultisampleStateCI,
			 .pDepthStencilState = pBlueprint->pDepthStencilStateCI, // Optional
			 .pColorBlendState = pBlueprint->pColourBlendStateCI,
			 .pDynamicState = pBlueprint->pDynamicStateCI,
			 .layout = layout,
			 .renderPass = pBlueprint->renderPassHandle,
			 .subpass = 0,
			 .basePipelineHandle = VK_NULL_HANDLE, // Optional
			 .basePipelineIndex = -1 // Optional
		};
		if(vkCreateGraphicsPipelines(devices->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS){
			 throw std::runtime_error("failed to create graphics pipeline!");
		}
	}
	~RenderPipeline(){
		vkDestroyPipeline(devices->GetLogicalDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(devices->GetLogicalDevice(), layout, nullptr);
//		vkDestroyShaderModule(devices->GetLogicalDevice(), fragShaderModule, nullptr);
//		vkDestroyShaderModule(devices->GetLogicalDevice(), vertShaderModule, nullptr);
	}
	
	// Bind the pipeline for subsequent render calls
	void CmdBind(VkCommandBuffer commandBuffer) const {
		vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
	}
	
	// Set which descriptor sets are bound for subsequent render calls
	template <uint32_t first=0, uint32_t number=0>
	bool CmdBindDescriptorSets(VkCommandBuffer commandBuffer, uint32_t flight, const std::vector<int> &dynamicOffsetNumbers=std::vector<int>()) {
		constexpr uint32_t numberUse = number < 1 ? descriptorSetCount - first : number;
		
		// making sure all descriptor sets are valid
		if(!uniforms.template UpdateDescriptorSets<first, numberUse>()){
			return false;
		}
		
		const VkDescriptorSet *const firstDescriptorSet = uniforms.DescriptorSetsStart(flight);
		
		// binding, optionally using dynamic offsets
		if(dynamicOffsetNumbers.empty()){
			vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, first, numberUse, firstDescriptorSet, 0, nullptr);
		} else {
			const std::vector<uint32_t> dynamicOffsets = uniforms.template GetDynamicOffsets<first, numberUse>(dynamicOffsetNumbers);
			vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, first, numberUse, firstDescriptorSet, uint32_t(dynamicOffsets.size()), dynamicOffsets.data());
		}
		return true;
	}
	
	template <uint32_t index>
	using pushConstantWithShaderStage_t = typename pushConstantManager_t::template pushConstantWithShaderStage_t<index>;
	
	template <uint32_t index>
	using pushConstant_t = typename pushConstantWithShaderStage_t<index>::type;
	
	template <uint32_t index>
	using pushConstantData_t = typename pushConstant_t<index>::type;
	
	// Set push constant data
	template <uint32_t index>
	void CmdPushConstants(VkCommandBuffer commandBuffer, pushConstantData_t<index> *data){
		vkCmdPushConstants(commandBuffer,
						   layout,
						   pushConstantWithShaderStage_t<index>::stageFlagsValue,
						   pushConstant_t<index>::offsetValue,
						   sizeof(pushConstantData_t<index>),
						   data);
	}
	
	// Get the handle of a descriptor set
	template <uint32_t index>
	uniforms_t::template descriptorSet_t<index> &iDescriptorSet(){ return uniforms.template iDescriptorSet<index>(); }
	
protected:
	std::shared_ptr<Devices> devices;
	
	uniforms_t uniforms;
	
	VkPipelineLayout layout;
	
	VkPipeline pipeline;
	
	static constexpr VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};



template <typename vertexShader_t>
requires (vertexShader_c<vertexShader_t>)
class DepthPipeline {
public:
	using uniforms_t = Uniforms<typename vertexShader_t::uniformWithShaderStage_tp>;
	
	using pushConstantManager_t = PushConstantManager<typename vertexShader_t::pushConstantWithShaderStage_tp>;
	
	static constexpr uint32_t descriptorSetCount = uniforms_t::descriptorSetCount;
	
	DepthPipeline(std::shared_ptr<Devices> _devices, const RenderPipelineBlueprint *const pBlueprint)
	: devices(_devices), uniforms(_devices) {
		// pipeline layout
		std::array<VkPushConstantRange, pushConstantManager_t::pushConstantCount> pcrs = pushConstantManager_t::PushConstantRanges();
		const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = uniforms_t::descriptorSetCount, // Optional
			.pSetLayouts = uniforms.DescriptorSetLayouts().data(), // Optional
			.pushConstantRangeCount = pushConstantManager_t::pushConstantCount,
			.pPushConstantRanges = pushConstantManager_t::pushConstantCount == 0 ? nullptr : pcrs.data()
		};
		if(vkCreatePipelineLayout(_devices->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS){
			throw std::runtime_error("Failed to create pipeline layout!");
		}
		
		// ----- Input assembly info -----
		const VkPipelineInputAssemblyStateCreateInfo inputAssembly {
			 .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			 .topology = pBlueprint->primitiveTopology,
			 .primitiveRestartEnable = VK_FALSE
		};
		// ----- Viewport state -----
		const VkPipelineViewportStateCreateInfo viewportState {
			 .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			 .viewportCount = 1,
			 .scissorCount = 1
		};
		// Shader stages
		const VkPipelineShaderStageCreateInfo stageCI = {// vertex shader
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.pName = "main",
			.module = devices->CreateShaderModule(vertexShader_t::filenameValue.data()),
			// this is for constants to use in the shader:
			.pSpecializationInfo = nullptr
		};
		
		const PipelineVertexInputStateCreateInfoSafe pviscis = vertexShader_t::PipelineVertexInputStateCI();
		const VkPipelineVertexInputStateCreateInfo pvisci = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			nullptr,
			NULL,
			uint32_t(pviscis.binding.size()),
			pviscis.binding.data(),
			uint32_t(pviscis.attribute.size()),
			pviscis.attribute.data()
		};
		
		// Pipeline
		const VkGraphicsPipelineCreateInfo pipelineInfo {
			 .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			 .stageCount = 1,
			 .pStages = &stageCI,
			 .pVertexInputState = &pvisci,//vertexShader_t::pipelineVertexInputStateCI,
			 .pInputAssemblyState = &inputAssembly,
			 .pViewportState = &viewportState,
			 .pRasterizationState = pBlueprint->pRasterisationStateCI,
			 .pMultisampleState = pBlueprint->pMultisampleStateCI,
			 .pDepthStencilState = pBlueprint->pDepthStencilStateCI, // Optional
			 .pColorBlendState = pBlueprint->pColourBlendStateCI,
			 .pDynamicState = pBlueprint->pDynamicStateCI,
			 .layout = layout,
			 .renderPass = pBlueprint->renderPassHandle,
			 .subpass = 0,
			 .basePipelineHandle = VK_NULL_HANDLE, // Optional
			 .basePipelineIndex = -1 // Optional
		};
		if(vkCreateGraphicsPipelines(devices->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS){
			 throw std::runtime_error("failed to create graphics pipeline!");
		}
	}
	~DepthPipeline(){
		vkDestroyPipeline(devices->GetLogicalDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(devices->GetLogicalDevice(), layout, nullptr);
//		vkDestroyShaderModule(devices->GetLogicalDevice(), fragShaderModule, nullptr);
//		vkDestroyShaderModule(devices->GetLogicalDevice(), vertShaderModule, nullptr);
	}
	
	// Bind the pipeline for subsequent render calls
	void CmdBind(VkCommandBuffer commandBuffer) const {
		vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
	}
	
	// Set which descriptor sets are bound for subsequent render calls
	template <uint32_t first=0, uint32_t number=0>
	bool CmdBindDescriptorSets(VkCommandBuffer commandBuffer, uint32_t flight, const std::vector<int> &dynamicOffsetNumbers=std::vector<int>()) {
		constexpr uint32_t numberUse = number < 1 ? descriptorSetCount - first : number;
		
		// making sure all descriptor sets are valid
		if(!uniforms.template UpdateDescriptorSets<first, numberUse>()){
			return false;
		}
		
		const VkDescriptorSet *const firstDescriptorSet = uniforms.DescriptorSetsStart(flight);
		
		// binding, optionally using dynamic offsets
		if(dynamicOffsetNumbers.empty()){
			vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, first, numberUse, firstDescriptorSet, 0, nullptr);
		} else {
			const std::vector<uint32_t> dynamicOffsets = uniforms.template GetDynamicOffsets<first, numberUse>(dynamicOffsetNumbers);
			vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, first, numberUse, firstDescriptorSet, uint32_t(dynamicOffsets.size()), dynamicOffsets.data());
		}
		return true;
	}
	
	template <uint32_t index>
	using pushConstantWithShaderStage_t = typename pushConstantManager_t::template pushConstantWithShaderStage_t<index>;
	
	template <uint32_t index>
	using pushConstant_t = typename pushConstantWithShaderStage_t<index>::type;
	
	template <uint32_t index>
	using pushConstantData_t = typename pushConstant_t<index>::type;
	
	// Set push constant data
	template <uint32_t index>
	void CmdPushConstants(VkCommandBuffer commandBuffer, pushConstantData_t<index> *data){
		vkCmdPushConstants(commandBuffer,
						   layout,
						   pushConstantWithShaderStage_t<index>::stageFlagsValue,
						   pushConstant_t<index>::offsetValue,
						   sizeof(pushConstantData_t<index>),
						   data);
	}
	
	// Get the handle of a descriptor set
	template <uint32_t index>
	uniforms_t::template descriptorSet_t<index> &iDescriptorSet(){ return uniforms.template iDescriptorSet<index>(); }
	
protected:
	std::shared_ptr<Devices> devices;
	
	uniforms_t uniforms;
	
	VkPipelineLayout layout;
	
	VkPipeline pipeline;
	
	static constexpr VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};



//template <typename computeShader_t>
//requires(shader_c<computeShader_t>)
//using computePipelineBase_t = PipelineBase<
//Uniforms<typename computeShader_t::uniforms_tp>,
//typename computeShader_t::pushConstants_tp
//>;
//
//template <typename computeShader_t>
//requires(shader_c<computeShader_t>)
//class ComputePipeline : public computePipelineBase_t<computeShader_t> {
//public:
//	using PipelineBase = typename ComputePipeline::PipelineBase;
//	
//	ComputePipeline(std::shared_ptr<Devices> _devices)
//	: PipelineBase(_devices) {
//		
//		// Shader stage
//		const VkPipelineShaderStageCreateInfo stageCI = {
//			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
//			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
//			.pName = "main",
//			.module = PipelineBase::devices->CreateShaderModule(computeShader_t::filename),
//			// this is for constants to use in the shader:
//			.pSpecializationInfo = nullptr
//		};
//		// Pipeline
//		VkComputePipelineCreateInfo pipelineInfo{
//			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
//			.stage = stageCI,
//			.layout = PipelineBase::layout
//		};
//		if(vkCreateComputePipelines(PipelineBase::devices->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &(PipelineBase::pipeline)) != VK_SUCCESS){
//			throw std::runtime_error("Failed to create compute pipeline!");
//		}
//	}
//	~ComputePipeline(){
////		vkDestroyShaderModule(devices->GetLogicalDevice(), shaderModule, nullptr);
//	}
//	
//protected:
//	VkPipelineBindPoint BindPoint() const override { return VK_PIPELINE_BIND_POINT_COMPUTE; }
//};

} // namespace EVK
