////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : FVTypeTraits.h											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// This file contains the struct "TypeTrait_FVal" which is used with std::visit
// to determine the category of a templated type in a FVal. example : TypeTrait_FVal<T>::is_basic
////------------------------------------------------------////

#pragma once
#include "Types.h"
#include <type_traits>

namespace Moonshot::fv_util
{
	// Index utility function
	inline constexpr bool isBasic(const std::size_t& t)
	{
		switch (t)
		{
			case indexes::fval_int:
			case indexes::fval_float:
			case indexes::fval_char:
			case indexes::fval_bool:
			case indexes::fval_str:
				return true;
			default:
				return false;
		}
	}
	inline constexpr bool isArithmetic(const std::size_t& t)
	{
		return (t == indexes::fval_int) || (t == indexes::fval_bool) || (t == indexes::fval_int);
	}
	
	inline constexpr bool isValue(const std::size_t& t)
	{
		return isBasic(t) || (t == indexes::fval_varRef);
	}

	inline constexpr bool canConcat(const std::size_t& lhs, const std::size_t& rhs)
	{
		if (isBasic(lhs) && isBasic(rhs))
		{
			switch (lhs)
			{
				case indexes::fval_char:
					return	((lhs == rhs)				||	// char & char
							(rhs == indexes::fval_str));	// char & str
				case indexes::fval_str:
					return	((lhs == rhs)				||	// str & str
						(rhs == indexes::fval_char));		// str & char
				default:
					return false;
			}
		}
		return false;
	}

	template <typename T>
	struct typeIndex
	{
		constexpr static std::size_t index = indexes::invalid_index;
	};
	// Specializations
	// monostate
	template<>
	struct typeIndex<std::monostate>
	{
		constexpr static std::size_t index = indexes::fval_null;
	};
	// int
	template<>
	struct typeIndex<IntType>
	{
		constexpr static std::size_t index = indexes::fval_int;
	};
	// float
	template<>
	struct typeIndex<float>
	{
		constexpr static std::size_t index = indexes::fval_float;
	};
	// char
	template<>
	struct typeIndex<CharType>
	{
		constexpr static std::size_t index = indexes::fval_char;
	};
	// string
	template<>
	struct typeIndex<std::string>
	{
		constexpr static std::size_t index = indexes::fval_str;
	};
	// bool
	template<>
	struct typeIndex<bool>
	{
		constexpr static std::size_t index = indexes::fval_bool;
	};
	// monostate
	template<>
	struct typeIndex<Moonshot::var::varRef>
	{
		constexpr static std::size_t index = indexes::fval_varRef;
	};
	// FValue traits class, to use with templated functions.
	template <typename T>
	struct TypeTrait_FVal : public typeIndex<T>
	{
		using typeIndex<T>::index;

		constexpr static bool is_basic = isBasic(index);
		constexpr static bool is_arithmetic = isArithmetic(index);
	};
}