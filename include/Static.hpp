#pragma once

#include <tuple>
#include <optional>
#include <type_traits>
#include <set>

#include <iostream>
#include <tuple>
#include <type_traits>

#include "Header.hpp"

namespace EVK {

// Type parameter pack
template <typename... Ts> struct TypePack {
	static constexpr bool empty = false;
};
template <> struct TypePack<> {
	static constexpr bool empty = true;
};
template <typename T>
concept typePack_c = requires {
	{T::empty} -> std::same_as<const bool &>;
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
template <typename... Ts> using concatenatedPack_t = typename ConcatenatedPack<Ts...>::type;

template <uint32_t index, typename... Ts> using IndexT = std::tuple_element<index, std::tuple<Ts...>>;

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

} // namespace EVK
