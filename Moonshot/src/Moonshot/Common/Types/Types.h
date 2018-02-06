////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Types.h											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// This file declares types/objects/typedefs specific to the interpreter.
// FVal, var_attr, etc.
// 
// This file also declares various helper function to analyze said types 
////------------------------------------------------------////

#pragma once

#include <variant> // std::variant
#include <string> // std::string
#include <sstream> // std::stringstream
#include <type_traits> // std::is_same
#include <memory>
#include <inttypes.h>
#include "../../Common/Exceptions/Exceptions.h"
#include "../../Fox/Util/Enums.h"

// fwd decl
namespace Moonshot::var
{
	struct varRef;
	struct varattr;
}

typedef int64_t IntType;
typedef wchar_t CharType;
typedef std::variant<std::monostate, IntType, float, CharType, std::string, bool, Moonshot::var::varRef> FVal;

namespace Moonshot
{
	namespace limits
	{
		static constexpr IntType IntType_MAX = INT64_MAX;
		static constexpr IntType IntType_MIN = INT64_MIN;
		static constexpr CharType CharType_MAX = WCHAR_MAX;
		static constexpr CharType CharType_MIN = WCHAR_MIN;
	}
	namespace fv_util
	{
		
		// Dump functions
		std::string dumpFVal(const FVal &var);
		std::string dumpVAttr(const var::varattr &var);

		// returns a sample fval for an index.
		FVal getSampleFValForIndex(const std::size_t& t);

		// Get a user friendly name for an index.
		std::string indexToTypeName(const std::size_t& t);

		// Index utility function
		bool isBasic(const std::size_t& t); // Is the type a string/bool/char/int/float ?
		bool isArithmetic(const std::size_t& t);
		bool isValue(const std::size_t& t);

		// Checks if assignement is possible.
		bool canAssign(const std::size_t &lhs, const std::size_t &rhs); // Checks if the lhs and rhs are compatible.
																		// Compatibility : 
																		// Arithmetic type <-> Arithmetic Type = ok
																		// string <-> string = ok
																		// else : error.

		// This function returns true if the type of basetype can be cast to the type of goal.
		bool canCastTo(const std::size_t &goal, const std::size_t &basetype);

		// returns the type of the biggest of the 2 arguments.
		std::size_t getBiggest(const std::size_t &lhs, const std::size_t &rhs);

		// Variables : Indexes
		// How to remember values of index
		namespace indexes
		{
			static constexpr std::size_t invalid_index = (std::numeric_limits<std::size_t>::max)();

			static constexpr std::size_t fval_null = 0;
			static constexpr std::size_t fval_int = 1;
			static constexpr std::size_t fval_float = 2;
			static constexpr std::size_t fval_char = 3;
			static constexpr std::size_t fval_str = 4;
			static constexpr std::size_t fval_bool = 5;
			static constexpr std::size_t fval_varRef = 6;
		}

		const std::map<std::size_t, std::string> kType_dict =
		{
			{ indexes::fval_null			, "NULL" },
			{ indexes::fval_int				, "INT" },
			{ indexes::fval_float			, "FLOAT" },
			{ indexes::fval_char			, "CHAR" },
			{ indexes::fval_bool			, "BOOL" },
			{ indexes::fval_str				, "STRING" },
			{ indexes::fval_varRef			, "VAR_ATTR (ref)"},
			{ indexes::invalid_index		, "!INVALID!" }
		};
	}

	namespace var
	{
		struct varRef
		{
			public:
				varRef(const std::string& vname = "");
				std::string getName() const;
				void setName(const std::string& newname);
				operator bool() const;  // checks validity of reference (if name != "");
			private:
				std::string name_;
		};
		struct varattr // Struct holding a var's attributes
		{
			varattr();
			varattr(const std::string &nm);
			varattr(const std::string &nm, const std::size_t &ty, const bool &isK = false);
			operator bool() const;
			// Variable's attribute
			bool isConst = false;
			std::string name_ = "";
			std::size_t type_ = fv_util::indexes::fval_null;

			varRef createRef() const;

			private:
				bool wasInit_ = false;
		};
		inline bool operator < (const varattr& lhs, const varattr& rhs)
		{
			return lhs.name_ < rhs.name_; // We don't care about the rest, because you can only use a name once.
		}
		inline bool operator == (const varattr& lhs, const varattr& rhs)
		{
			return (lhs.name_ == rhs.name_) &&
				(lhs.isConst == rhs.isConst) &&
				(lhs.type_ == rhs.type_);
		}
		inline bool operator != (const varattr& lhs, const varattr& rhs)
		{
			return !(lhs == rhs);
		}
	}
}