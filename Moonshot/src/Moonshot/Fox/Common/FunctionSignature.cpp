////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : FunctionSignature.cpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
////------------------------------------------------------////

#include "FunctionSignature.hpp"

using namespace Moonshot::fn;

argattr::argattr(const std::string & nm, const std::size_t & ty, const bool isK, const bool & isref)
{
	name_ = nm;
	type_ = ty;
	isRef_ = isref;
	wasInit_ = true;
}

argattr::operator bool() const
{
	return (wasInit_ && (type_ != TypeIndex::Null_Type) && (type_ != TypeIndex::InvalidIndex));
}

FunctionSignature::operator bool() const
{
	if ((name_ != "") && (returnType_ != TypeIndex::InvalidIndex))
	{
		for (const auto& elem : args_)
		{
			if (!elem)
				return false;
		}
	}
	return true;
}
