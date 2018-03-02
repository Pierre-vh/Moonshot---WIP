////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : TypesUtils.cpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
////------------------------------------------------------////

#include "TypesUtils.hpp"
#include <sstream>
#include "../UTF8/StringManipulator.hpp"

using namespace Moonshot;

std::string TypeUtils::dumpFVal(const FVal & var)
{
	std::stringstream output;
	if (std::holds_alternative<NullType>(var))
		output << "Type : VOID (null)";
	else if (std::holds_alternative<var::varRef>(var))
	{
		auto vattr = std::get<var::varRef>(var);
		output << "Type : varRef, Value:" << vattr.getName();
	}
	else if (std::holds_alternative<IntType>(var))
		output << "Type : INT, Value : " << std::get<IntType>(var);
	else if (std::holds_alternative<float>(var))
		output << "Type : FLOAT, Value : " << std::get<float>(var);
	else if (std::holds_alternative<std::string>(var))
		output << "Type : STRING, Value : \"" << std::get<std::string>(var) << "\"";
	else if (std::holds_alternative<bool>(var))
	{
		const bool v = std::get<bool>(var);
		output << "Type : BOOL, Value : " << (v ? "true" : "false");
	}
	else if (std::holds_alternative<CharType>(var))
	{
		CharType x = std::get<CharType>(var);
		UTF8::StringManipulator u8sm;
		output << "Type : CHAR, Value : " << x << " = '" << u8sm.wcharToStr(x) << "'";
	}
	else
		throw std::logic_error("Illegal variant.");
	return output.str();
}
std::string TypeUtils::dumpVAttr(const var::varattr & var)
{
	std::stringstream output;
	output << "[name:\"" << var.name_ << "\" "
		<< "type: " << (var.isConst_ ? "CONST " : "");
	auto friendlyname = kType_dict.find(var.type_);
	if (friendlyname != kType_dict.end())
		output << friendlyname->second;
	else
		output << "<UNKNOWN>";
	output << "]";
	return output.str();
}
FVal TypeUtils::getSampleFValForIndex(const std::size_t & t)
{
	switch (t)
	{
	case indexes::fval_null:
		return FVal();
	case indexes::fval_int:
		return FVal((IntType)0);
	case indexes::fval_float:
		return FVal((float)0.0f);
	case indexes::fval_char:
		return FVal((CharType)0);
	case indexes::fval_str:
		return FVal(std::string(""));
	case indexes::fval_bool:
		return FVal((bool)false);
	case indexes::fval_varRef:
		return FVal(var::varattr());
	case indexes::invalid_index:
		throw std::logic_error("Tried to get a sample FVal with an invalid index");
		return FVal();
	default:
		throw std::logic_error("Defaulted while attempting to return a sample FVal for an index. -> Unknown index. Unimplemented type?");
		return FVal();
	}
}

std::string TypeUtils::indexToTypeName(const std::size_t & t)
{
	auto a = kType_dict.find(t);
	if (a != kType_dict.end())
		return a->second;
	return "!IMPOSSIBLE_TYPE!";
}

bool TypeUtils::canAssign(const std::size_t & lhs, const std::size_t & rhs)
{
	if ((rhs == indexes::fval_null) || (lhs == indexes::fval_null))
		return false; // Can't assign a void expression to a variable.
	if (!isBasic(lhs) || !isBasic(rhs))
		// If one of the types isn't basic, no assignement possible.
		return false;
	else if (lhs == rhs) // same type to same type = ok.
		return true;
	// From here, we know lhs and rhs are different.
	else if (!isArithmetic(lhs) || !isArithmetic(rhs)) // one of them is a string
		return false;  // Can't assign a string to an arithmetic type and vice versa.
					   // Else, we're good, return true.
	return true;
}
bool TypeUtils::canImplicitelyCastTo(const std::size_t & goal, const std::size_t & basetype)
{
	/*
		Implicit Conversions:
		Arith type -> Arith type
		char type -> string type
		same type -> same type
	*/
	if (isBasic(basetype))
	{
		if (isArithmetic(goal) && isArithmetic(basetype)) // arith -> arith
			return true;
		else if ((basetype == indexes::fval_char) && (goal == indexes::fval_str)) // char -> str
			return true;
		return (basetype == goal); // same type -> same type
	}
	return false;
}

bool TypeUtils::canExplicitelyCastTo(const std::size_t & goal, const std::size_t & basetype)
{
	/*
		Implicit rules + strings <-> arith support (string as int to interpret it as int, int as string to convert it to string)
	*/
	if (canImplicitelyCastTo(goal, basetype))
		return true;
	else
	{
		return	((goal == indexes::fval_str) && isArithmetic(basetype)) || // arith -> str
				(isArithmetic(goal) && (basetype == indexes::fval_str)); // str -> arith
	}
	return false;
}

std::size_t TypeUtils::getBiggest(const std::size_t & lhs, const std::size_t & rhs)
{
	if (isArithmetic(lhs) && isArithmetic(rhs))
	{
		if ((lhs == indexes::fval_float) || (rhs == indexes::fval_float))
			return indexes::fval_float;
		else if ((lhs == indexes::fval_int) || (rhs == indexes::fval_int))
			return indexes::fval_int;
		else if ((lhs == indexes::fval_char) || (rhs == indexes::fval_char))
			return indexes::fval_char;
		else
			return indexes::fval_bool;
	}
	else
		throw std::logic_error("Can't return the biggest of two types when one of the two type isn't arithmetic.");
	return indexes::invalid_index;
}
