#pragma once

#include <tuple>
#include <optional>
#include <type_traits>
#include <set>

#include <Base.hpp>

#include <iostream>
#include <tuple>
#include <type_traits>



namespace Static {


// Tools
// -----
// Type parameter pack
template <typename... Ts> struct TypePack {
	static constexpr bool empty = false;
};
template <> struct TypePack<> {
	static constexpr bool empty = true;
};

// Type pack concatenation
template <typename... Ts> struct ConcatenatedPack;
template <> struct ConcatenatedPack<> {
	using type = TypePack<>;
};
template <typename... Ts> struct ConcatenatedPack<TypePack<Ts...>> {
	using type = TypePack<Ts...>;
};
template <typename... Ts, typename... Us> struct ConcatenatedPack<TypePack<Ts...>, TypePack<Us...>> {
	using type = TypePack<Ts..., Us...>;
};
template<typename... Ts, typename... Us, typename... Rest> struct ConcatenatedPack<TypePack<Ts...>, TypePack<Us...>, Rest...> {
	using type = typename ConcatenatedPack<TypePack<Ts..., Us...>, Rest...>::type;
};
template <typename... Ts> using ConcatenatedPack_t = typename ConcatenatedPack<Ts...>::type;

template <uint32_t index, typename... Ts> using IndexT = std::tuple_element<index, std::tuple<Ts...>>;

template <typename... Ts> consteval uint32_t CountT() { return std::tuple_size<std::tuple<Ts...>>::value; }

template <uint32_t integer, uint32_t... integers> consteval bool Contains(){
	return ((integer == integers) || ...);
}
template <uint32_t integer> consteval bool Unique(){ return true; }
template <uint32_t integer1, uint32_t integer2, uint32_t... integers> consteval bool Unique(){
	return !Contains<integer1, integer2, integers...>() && Unique<integer2, integers...>();
}

template <typename integralT>
constexpr integralT RoundUp(integralT numToRound, integralT multiple){
	if (multiple == 0)
		return numToRound;
	integralT remainder = numToRound % multiple;
	if (remainder == 0)
		return numToRound;
	return numToRound + multiple - remainder;
}

template <typename integralT>
constexpr bool IsMultipleOf(integralT num, integralT of){
	return (num % of) == 0;
}

template <typename T> consteval uint32_t AttributeSizeInShader(){ return RoundUp<uint32_t>(sizeof(T), 16) / 16; }


// Push constants
// -----
struct NoPushConstants {};
template <typename T> struct PushConstants {};


// Descriptor
// -----
template <uint32_t binding, VkShaderStageFlags stageFlags> struct DescriptorBase {
	static constexpr uint32_t bindingValue = binding;
	static constexpr VkShaderStageFlags stageFlagsValue = stageFlags;
	
	virtual VkDescriptorSetLayoutBinding LayoutBinding() const = 0;
	
	virtual VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const = 0;
	
	virtual VkDescriptorPoolSize PoolSize() const = 0;
};

template <uint32_t binding, VkShaderStageFlags stageFlags>
struct UBODescriptor : public DescriptorBase<binding, stageFlags> {
	int GetIndex() const { return index; }
	void SetIndex(int newValue){ index = newValue; }
	
	Dynamic GetDynamic() const { return dynamic; }
	void SetDynamic(const Dynamic &newValue){ dynamic = newValue; }
	
	struct Dynamic {
		int repeatsN;
		VkDeviceSize alignment;
	};
	
	VkDescriptorSetLayoutBinding LayoutBinding() const override;
	
	VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
	
	VkDescriptorPoolSize PoolSize() const override;
	
private:
	int index;
	std::optional<Dynamic> dynamic;
};

template <uint32_t binding, VkShaderStageFlags stageFlags>
struct SBODescriptor : public DescriptorBase<binding, stageFlags> {
	int GetIndex() const { return index; }
	void SetIndex(int newValue){ index = newValue; }
	
	int GetFlightOffset() const { return flightOffset; }
	void SetFlightOffset(int newValue){ flightOffset = newValue; }
	
	VkDescriptorSetLayoutBinding LayoutBinding() const override;
	
	VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
	
	VkDescriptorPoolSize PoolSize() const override;
	
private:
	int index;
	int flightOffset;
};


template <uint32_t binding, VkShaderStageFlags stageFlags, >
struct TextureImagesDescriptor : public DescriptorBase<binding, stageFlags> {
	VkImage image;
	VmaAllocation allocation;
	VkImageView view;
	uint32_t mipLevels;
	ManualImageBlueprint blueprint;
	
	void CleanUp(const VkDevice &logicalDevice, const VmaAllocator &allocator){
		vkDestroyImageView(logicalDevice, view, nullptr);
		vmaDestroyImage(allocator, image, allocation);
	}
	
	VkDescriptorSetLayoutBinding LayoutBinding() const override;
	
	VkWriteDescriptorSet DescriptorWrite(const VkDescriptorSet &dstSet, VkDescriptorImageInfo *imageInfoBuffer, int &imageInfoBufferIndex, VkDescriptorBufferInfo *bufferInfoBuffer, int &bufferInfoBufferIndex, int flight) const override;
	
	VkDescriptorPoolSize PoolSize() const override;
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

template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags>
struct TextureImage : public UniformBase<set, binding, stageFlags> {};

template <uint32_t set, uint32_t binding, VkShaderStageFlags stageFlags>
struct TextureSampler : public UniformBase<set, binding, stageFlags> {};

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
};


// Uniforms
// -----
template <typename... uniformTs> struct Uniforms {
private:
	template <uint32_t set> static consteval
	bool SetExists(){ return !FilteredDescriptorPack<set, uniformTs...>::empty; }
	
	template <uint32_t set> static consteval
	uint32_t SetCountImpl(){
		if constexpr (SetExists<set>())
			return SetCountImpl<set + 1>();
		else
			return set;
	}
	
	template <uint32_t set> static consteval
	bool AllSmallerSetsExist(){ return SetExists<set>() && AllSmallerSetsExist<set - 1>(); }
	template <> static consteval
	bool AllSmallerSetsExist<0>(){ return SetExists<0>(); }
	
public:
	static_assert(UniformUnique<uniformTs...>(), "No two uniforms should have both matching set and binding.");
	
	static_assert((AllSmallerSetsExist<uniformTs::setValue>() && ...), "The union of descriptor set indices for a shader should be consecutive and contain 0.");
	
	template <uint32_t set> requires (SetExists<set>())
	using iDescriptorSet = DescriptorSet<FilteredDescriptorPack<set, uniformTs...>>;
	
	static constexpr uint32_t descriptorSetCount = SetCountImpl<0>();
};


// Shaders
// -----
template <typename PushConstantsT, typename AttributesT, typename UniformsT> struct Shader {
	PushConstantsT pushConstants;
	AttributesT attributes;
	UniformsT uniforms;
};

Shader<
NoPushConstants,
Attributes<
	TypePack<vec<3>, vec<3>, vec<2>, mat<4, 4>, mat<4, 4>>,
	BindingDescriptionPack<
		VkVertexInputBindingDescription{
			0, // binding
			32,//sizeof(Vertex), // stride
			VK_VERTEX_INPUT_RATE_VERTEX // input rate
		},
		VkVertexInputBindingDescription{
			1, // binding
			128,//sizeof(Vertex), // stride
			VK_VERTEX_INPUT_RATE_INSTANCE // input rate
		}
	>,
	AttributeDescriptionPack<
		VkVertexInputAttributeDescription{
			0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0//offsetof(Vertex, position)
		},
		VkVertexInputAttributeDescription{
			1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12//offsetof(Vertex, normal)
		},
		VkVertexInputAttributeDescription{
			2, 0, VK_FORMAT_R32G32_SFLOAT, 24//offsetof(Vertex, texCoord)
		},
		VkVertexInputAttributeDescription{
			3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0//offsetof(Vertex, position)
		},
		VkVertexInputAttributeDescription{
			4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16//offsetof(Vertex, position)
		},
		VkVertexInputAttributeDescription{
			5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32//offsetof(Vertex, position)
		},
		VkVertexInputAttributeDescription{
			6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 48//offsetof(Vertex, position)
		},
		VkVertexInputAttributeDescription{
			7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 64//offsetof(Vertex, normal)
		},
		VkVertexInputAttributeDescription{
			8, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 80//offsetof(Vertex, normal)
		},
		VkVertexInputAttributeDescription{
			9, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 96//offsetof(Vertex, normal)
		},
		VkVertexInputAttributeDescription{
			10, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 112//offsetof(Vertex, normal)
		}
>>,
Uniforms<
	UBO<0, 0, VK_SHADER_STAGE_FRAGMENT_BIT, float>,
	TextureImage<0, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT>
>>
shader;

} // namespace Static
