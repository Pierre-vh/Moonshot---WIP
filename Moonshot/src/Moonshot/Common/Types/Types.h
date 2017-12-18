#pragma once

#include <variant> // std::variant
#include <string> // std::string
#include <sstream> // std::stringstream
#include <type_traits> // std::is_same

#include "../../Common/Errors/Errors.h"
#include "../../Fox/Util/Enums.h"

// fwd decl
namespace Moonshot::var
{
	struct varattr;
}


// Alias for a variant holding every type possible in the interpreter.
typedef void* FVAL_NULLTYPE;
typedef std::variant<FVAL_NULLTYPE,int, float, char, std::string, bool, Moonshot::var::varattr> FVal;


namespace Moonshot
{
	namespace fv_util
	{
		template <typename T>
		struct fval_traits
		{
			constexpr static bool isBasic =
					std::is_same<T,int>::value				||
					std::is_same<T,float>::value			||
					std::is_same<T,char>::value				||
					std::is_same<T,std::string>::value		||
					std::is_same<T,bool>::value
				;
			constexpr static bool isArithmetic = isBasic && !std::is_same<T, std::string>::value;
		};
		std::string dumpFVal(const FVal &var);

		FVal getSampleFValForIndex(const std::size_t& t);

		std::string indexToTypeName(const std::size_t& t);

		bool isBasic(const std::size_t& t); // Is the type a string/bool/char/int/float ?
		bool isArithmetic(const std::size_t& t);

		bool canAssign(const std::size_t &lhs, const std::size_t &rhs); // Checks if the lhs and rhs are compatible.
																		// Compatibility : 
																		// Arithmetic type <-> Arithmetic Type = ok
																		// string <-> string = ok
																		// else : error.

		// Thanks, I guess ! This looks healthier than using -1 as invalid index. https://stackoverflow.com/a/37126153
		static constexpr std::size_t invalid_index = std::numeric_limits<std::size_t>::max();
		// How to remember values of index
		static constexpr std::size_t fval_void = 0;
		static constexpr std::size_t fval_int = 1;
		static constexpr std::size_t fval_float = 2;
		static constexpr std::size_t fval_char = 3;
		static constexpr std::size_t fval_str = 4;
		static constexpr std::size_t fval_bool = 5;
		static constexpr std::size_t fval_vattr = 6;

		const std::map<std::size_t, std::string> kType_dict =
		{
			{ fval_void				, "NULL" },
			{ fval_int				, "INT" },
			{ fval_float			, "FLOAT" },
			{ fval_char				, "CHAR" },
			{ fval_bool				, "BOOL" },
			{ fval_str				, "STRING" },
			{ fval_vattr			, "VAR_ATTR (ref)"},
			{ invalid_index			, "!INVALID_FVAL!" }
		};
	}

	namespace var
	{
		struct varattr // Struct holding a var's attributes
		{
			varattr();
			varattr(const std::string &nm, const std::size_t &ty, const bool &isK = false);
			operator bool() const;
			// Variable's attribute
			bool isConst = false;
			std::string name = "";
			std::size_t type = fv_util::fval_void;

		private:
			bool wasInit_ = false;
		};
	}
}