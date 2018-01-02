////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Token.cpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
////------------------------------------------------------////

#include "Token.h"
#include "../../Common/Errors/Errors.h"

using namespace Moonshot;

token::token()
{
	empty_ = true;
}
token::token(std::string data, const text_pos &tpos) : str(data),pos(tpos)
{
	// self id
	selfId();
}

std::string token::showFormattedTokenData() const
{
	if (type == tokenType::TT_ENUM_DEFAULT && str == "") // Token isn't initialized.
		return "<EMPTY TOKEN>"; // return nothing.

	std::stringstream ss;
	ss << "[str:\"" << str << "\"][" << pos.asText() << "][type:";
	int enum_info = -1;		// The information of the corresponding enumeration
	switch (type)
	{
		case tokenType::TT_ENUM_DEFAULT:
			ss << "ENUM_DEFAULT";
			break;
		case tokenType::TT_IDENTIFIER:
			ss << "IDENTIFIER";
			enum_info = -2;
			break;
		case tokenType::TT_KEYWORD:
			ss << "KEYWORD";
			enum_info = util::enumAsInt(kw_type);
			break;
		case tokenType::TT_SIGN:
			ss << "SIGN";
			enum_info = util::enumAsInt(sign_type);
			break;
		case tokenType::TT_VALUE:
			ss << "VALUE";
			enum_info = util::enumAsInt(val_type);
			break;
	}
	if (enum_info >= -1)
		ss << " -> E:" << enum_info;
	ss << "]";
	return ss.str();
}
bool Moonshot::token::isValid() const
{
	return !empty_;
}
void token::selfId()
{
	if (!E_CHECKSTATE)
	{
		E_ERROR("Errors happened earlier, as a result tokens can't be identified.");
		return;
	}
	if (str.size() == 0)
		E_CRITICAL("Found an empty token. [" + pos.asText() + "]");

	// substract the token length's fron the column number given by the lexer.
	pos.column -= (int)str.length();

	if (idSign())
		type = tokenType::TT_SIGN;
	else
	{
		if (idKeyword())
			type = tokenType::TT_KEYWORD;
		else if (idValue())
			type = tokenType::TT_VALUE;
		else if (std::regex_match(str, kId_regex))
			type = tokenType::TT_IDENTIFIER;
		else
			E_ERROR("Could not identify a token (str) : " + str + "\t[" + pos.asText() + "]");
	}
}

bool token::idKeyword()
{
	auto i = kWords_dict.find(str);
	if (i == kWords_dict.end())
		return false;
	
	kw_type = i->second;
	return true;
}

bool token::idSign()
{
	if (str.size() > 1)
		return false;
	if (isdigit(str[0]))
		return false;
	auto i = kSign_dict.find(str[0]);
	if (i != kSign_dict.end())
	{
		sign_type = i->second;
		return true;
	}
	return false;
}

bool token::idValue()
{
	if (str[0] == '\'' )
	{
		if (str.back() == '\'')
		{
			str = str[1];
			if (str == "\\" && str.size() == 4) // If we have a \n in a char or something
				str += str[2];
			val_type = valueType::VAL_CHAR;
			return true;
		}
		else
		{
			E_ERROR("Unclosed char " + str);
			return false;
		}
	}
	else if (str[0] == '"')
	{
		if (str.back() == '"')
		{
			str = str.substr(1, str.size() - 2);
			val_type = valueType::VAL_STRING;
			return true;
		}
		else
		{
			E_ERROR("Unclosed string: " + str);
			return false;
		}
	}
	else if (str == "true" | str == "false")
	{
		vals = (str == "true" ? true : false);
		val_type = valueType::VAL_BOOL;
		return true;
	}
	else if (std::regex_match(str, kInt_regex))
	{
		vals = std::stoi(str);
		val_type = valueType::VAL_INTEGER;
		return true;
	}
	else if (std::regex_match(str, kFloat_regex))
	{
		vals = std::stof(str);
		val_type = valueType::VAL_FLOAT;
		return true;
	}
	return false;
}

text_pos::text_pos()
{
}

text_pos::text_pos(const int & l, const int & col) : line(l), column(col)
{

}

void text_pos::newLine()
{
	line ++;
	//	column = 0;
}

void text_pos::forward()
{
//	column += 1;
}

std::string text_pos::asText() const
{
	std::stringstream ss;
	ss << "LINE:" << line /*<< " C:" << column*/;
	return ss.str();
}
