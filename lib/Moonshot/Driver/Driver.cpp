////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Driver.cpp											
// Author : Pierre van Houtryve											
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
////------------------------------------------------------////

#include "Driver.hpp"

#include "Moonshot/Common/Context/Context.hpp"
#include "Moonshot/Fox/Lexer/Lexer.hpp"
#include "Moonshot/Fox/Parser/Parser.hpp"
#include "Moonshot/Fox/AST/Dumper.hpp"

#include <fstream>

using namespace Moonshot;

bool Driver::compileFunction(std::ostream& out, const std::string& filepath)
{
	Context ctxt(Context::LoggingMode::SAVE_TO_VECTOR);

	std::string filecontent;
	if (!readFileToString(filepath, filecontent))
	{
		out << "Could not open file \"" << filepath << "\"\n";
		return false;
	}

	Lexer lex(ctxt);
	lex.lexStr(filecontent);

	if (!ctxt.isSafe())
	{
		out << "Failed at lexing. Logs:\n";
		out << ctxt.getLogs();
		return false;
	}
	else
		out << "Lexing completed successfully." << lex.getTokenVector().size() << " tokens found.\n";

	Parser psr(ctxt,lex.getTokenVector());
	auto presult = psr.parseUnit();

	if (!presult)
	{
		out << "Failed at parsing. Logs:\n";
		out << ctxt.getLogs();
		if (!presult.isDataAvailable()) // no data? return now to avoid crash with dumper call.
		{
			out << "Parser returned no data.\n";
			return false;
		}
	}
	else
	{
		out << "Parsing successful!\n";
		out << ctxt.getLogs();
	}
	

	out << "\nAST Dump:\n";
	Dumper dump(out,1);
	dump.dumpUnit(*presult.result_);
	return true;
}

bool Driver::readFileToString(const std::string & filepath, std::string & outstr) const
{
	std::ifstream in(filepath, std::ios::binary); 	// read file
	if (in)
	{
		outstr = (std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
		return true;
	}
	return false;
}
