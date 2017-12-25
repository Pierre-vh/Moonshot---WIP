#include "Dumper.h"

using namespace Moonshot;
using namespace fv_util;

Dumper::Dumper()
{
	//std::cout << "Visitor \"Dumper\" Initialized. Dumping tree:" << std::endl;
}

Dumper::~Dumper()
{
}

void Dumper::visit(ASTExpr & node)
{
	std::cout << tabs() << "Expression : Operator ";
	// Attempts to print the operator in a str form
	auto strOp = parse::kOptype_dict.find(node.op_);
	if (strOp != parse::kOptype_dict.end())
		std::cout << strOp->second;
	else
		std::cout << node.op_;

	if (node.totype_ != invalid_index)
		std::cout << ", Return type : " << indexToTypeName(node.totype_);
	std::cout << std::endl;
	if (node.left_)
	{
		std::cout << tabs() << char(192) << " Left child:" << tabs() << std::endl;
		tabcount += 1;
		node.left_->accept(*this);
		tabcount -= 1;
	}
	if (node.right_)
	{
		std::cout << tabs() << char(192) << " Right child:" << tabs() << std::endl;
		tabcount += 1;
		node.right_->accept(*this);
		tabcount -= 1;
	}
}

void Dumper::visit(ASTValue & node)
{
	std::cout << tabs() << char(192) << "ExprValue: " << dumpFVal(node.val_) << std::endl;
}

void Dumper::visit(ASTVarDeclStmt & node)
{
	std::cout << tabs() << "VarDeclStmt :" << dumpVAttr(node.vattr_) << std::endl;
	if (node.initExpr_)
	{
		tabcount += 1;
		std::cout << tabs() << char(192) << "InitExpr" << std::endl;
		tabcount += 1;
		node.initExpr_->accept(*this);
		tabcount -= 2;
	}
}

std::string Dumper::tabs() const
{
	std::string i;
	for (unsigned int k(0); k < tabcount; k++)
		i += '\t';
	return i;
}


