////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : ASTNullStmt.hpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// A Simple struct representing a null statement.								
////------------------------------------------------------////

#pragma once

#include "IASTStmt.hpp"

namespace Moonshot
{
	struct ASTNullStmt : public IASTStmt	// A null statement, that doesn't do anything.
											// That's used by the parser, but doesn't actually appear in the AST.
	{
		virtual void accept(IVisitor& vis) { vis.visit(*this); }
	};
}