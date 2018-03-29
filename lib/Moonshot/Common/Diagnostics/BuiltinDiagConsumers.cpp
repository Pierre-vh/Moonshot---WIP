////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : BuiltinDiagConsumers.cpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
////------------------------------------------------------////

#include "BuiltinDiagConsumers.hpp"
#include <iostream>

using namespace Moonshot;

StreamDiagConsumer::StreamDiagConsumer(std::ostream & stream) : os(stream)
{

}

void StreamDiagConsumer::consume(const Diagnostic & diag)
{
	os << "[" << diagSevToString(diag.getDiagSeverity()) << "]\t\t" << diag.getDiagStr() << "\n";
}

std::string StreamDiagConsumer::diagSevToString(const DiagSeverity & ds) const
{
	switch (ds)
	{
		case DiagSeverity::IGNORE:
			return "Ignored";
		case DiagSeverity::NOTE:
			return "Note";
		case DiagSeverity::WARNING:
			return "Warning";
		case DiagSeverity::ERROR:
			return "Error";
		case DiagSeverity::FATAL:
			return "Fatal";
	}
	return "<Unknown Severity>";
}
