////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : ASTCompStmt.cpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
////------------------------------------------------------////

#include "ASTCompStmt.h"

using namespace Moonshot;

ASTCompStmt::ASTCompStmt()
{
}

ASTCompStmt::~ASTCompStmt()
{

}

void ASTCompStmt::accept(IVisitor & vis)
{
	vis.visit(*this);
}