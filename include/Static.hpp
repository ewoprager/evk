#pragma once

#include <tuple>
#include <optional>
#include <type_traits>
#include <set>

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

//
//
//Shader<
//NoPushConstants,
//Attributes<
//	TypePack<vec<3>, vec<3>, vec<2>, mat<4, 4>, mat<4, 4>>,
//	BindingDescriptionPack<
//		VkVertexInputBindingDescription{
//			0, // binding
//			32,//sizeof(Vertex), // stride
//			VK_VERTEX_INPUT_RATE_VERTEX // input rate
//		},
//		VkVertexInputBindingDescription{
//			1, // binding
//			128,//sizeof(Vertex), // stride
//			VK_VERTEX_INPUT_RATE_INSTANCE // input rate
//		}
//	>,
//	AttributeDescriptionPack<
//		VkVertexInputAttributeDescription{
//			0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0//offsetof(Vertex, position)
//		},
//		VkVertexInputAttributeDescription{
//			1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12//offsetof(Vertex, normal)
//		},
//		VkVertexInputAttributeDescription{
//			2, 0, VK_FORMAT_R32G32_SFLOAT, 24//offsetof(Vertex, texCoord)
//		},
//		VkVertexInputAttributeDescription{
//			3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0//offsetof(Vertex, position)
//		},
//		VkVertexInputAttributeDescription{
//			4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16//offsetof(Vertex, position)
//		},
//		VkVertexInputAttributeDescription{
//			5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32//offsetof(Vertex, position)
//		},
//		VkVertexInputAttributeDescription{
//			6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 48//offsetof(Vertex, position)
//		},
//		VkVertexInputAttributeDescription{
//			7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 64//offsetof(Vertex, normal)
//		},
//		VkVertexInputAttributeDescription{
//			8, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 80//offsetof(Vertex, normal)
//		},
//		VkVertexInputAttributeDescription{
//			9, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 96//offsetof(Vertex, normal)
//		},
//		VkVertexInputAttributeDescription{
//			10, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 112//offsetof(Vertex, normal)
//		}
//>>,
//Uniforms<
//	UBO<0, 0, VK_SHADER_STAGE_FRAGMENT_BIT, float>,
//	TextureImage<0, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT>
//>>
//shader;

} // namespace Static
