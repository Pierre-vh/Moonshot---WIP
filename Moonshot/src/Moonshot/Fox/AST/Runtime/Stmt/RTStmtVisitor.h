////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : RTStmtVisitor.h											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// Runtime visitor for statements.
// Used for debugging purposes. When the VM will be done, this file
// will probably be useless.
////------------------------------------------------------////
#pragma once
#include "../../../../Common/Utils/Utils.h"
#include "../../../../Common/Errors/Errors.h"
#include "../../../../Common/Types/Types.h"
#include "../../../../Common/Symbols/Symbols.h"

#include "../../Nodes/ASTExpr.h"
#include "../../Nodes/ASTVarDeclStmt.h"
#include "../Expr/RTExprVisitor.h"

#include "../IRTVisitor.h"

namespace Moonshot
{
	// Visits statements nodes : vardecl & exprstmt
	class RTStmtVisitor : public RTExprVisitor // Inherits from the expression visitor, because of expression statements!
	{
		public:
			RTStmtVisitor();
			RTStmtVisitor(std::shared_ptr<SymbolsTable> symtab);
			~RTStmtVisitor();

			virtual FVal visit(ASTVarDeclStmt & node) override;

		private:
			// Declares the value, but deref initival if it's a reference.
			bool symtab_declareValue_derefFirst(const var::varattr& vattr, FVal initval = FVal());
	};

}

