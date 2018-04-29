////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Stmt.cpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
////------------------------------------------------------////

#include "Expr.hpp"
#include "Expr.hpp"

#include "IVisitor.hpp"

using namespace Moonshot;

// Stmt
Stmt::Stmt(const StmtKind & skind) : kind_(skind)
{
}

StmtKind Stmt::getKind() const
{
	return kind_;
}

// return stmt
ReturnStmt::ReturnStmt(std::unique_ptr<Expr> rtr_expr) : Stmt(StmtKind::ReturnStmt)
{
	expr_ = std::move(rtr_expr);
}

void ReturnStmt::accept(IVisitor & vis)
{
	vis.visit(*this);
}

bool ReturnStmt::hasExpr() const
{
	return (bool)expr_;
}

Expr * ReturnStmt::getExpr()
{
	return expr_.get();
}

void ReturnStmt::setExpr(std::unique_ptr<Expr> e)
{
	expr_ = std::move(e);
}

// cond stmt
ConditionStmt::ConditionStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then, std::unique_ptr<Stmt> elsestmt) :
	Stmt(StmtKind::ConditionStmt)
{
	setCond(std::move(cond));
	setThen(std::move(then));
	setElse(std::move(elsestmt));
}

void ConditionStmt::accept(IVisitor & vis)
{
	vis.visit(*this);
}

bool ConditionStmt::isValid() const
{
	return cond_ && then_;
}

bool ConditionStmt::hasElse() const
{
	return (bool)else_;
}

Expr * ConditionStmt::getCond()
{
	return cond_.get();
}

Stmt * ConditionStmt::getThen()
{
	return then_.get();
}

Stmt * ConditionStmt::getElse()
{
	return else_.get();
}

void ConditionStmt::setCond(std::unique_ptr<Expr> expr)
{
	cond_ = std::move(expr);
}

void ConditionStmt::setThen(std::unique_ptr<Stmt> then)
{
	then_ = std::move(then);
}

void ConditionStmt::setElse(std::unique_ptr<Stmt> elsestmt)
{
	else_ = std::move(elsestmt);
}

// Compound stmt
CompoundStmt::CompoundStmt() : Stmt(StmtKind::CompoundStmt)
{

}

void CompoundStmt::accept(IVisitor & vis)
{
	vis.visit(*this);
}

Stmt * CompoundStmt::getStmt(const std::size_t & ind)
{
	if (ind > stmts_.size())
		throw std::out_of_range("out of range");

	return stmts_[ind].get();
}

Stmt * CompoundStmt::getBack()
{
	return stmts_.back().get();
}

void CompoundStmt::addStmt(std::unique_ptr<Stmt> stmt)
{
	stmts_.emplace_back(std::move(stmt));
}

bool CompoundStmt::isEmpty() const
{
	return !(stmts_.size());
}

std::size_t CompoundStmt::size() const
{
	return stmts_.size();
}

CompoundStmt::StmtVecIter CompoundStmt::stmts_beg()
{
	return stmts_.begin();
}

CompoundStmt::StmtVecIter CompoundStmt::stmts_end()
{
	return stmts_.end();
}

CompoundStmt::StmtVecConstIter CompoundStmt::stmts_beg() const
{
	return stmts_.begin();
}

CompoundStmt::StmtVecConstIter CompoundStmt::stmts_end() const
{
	return stmts_.end();
}

// While stmt
WhileStmt::WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body) :
	Stmt(StmtKind::WhileStmt)
{
	setCond(std::move(cond));
	setBody(std::move(body));
}

void WhileStmt::accept(IVisitor & vis)
{
	vis.visit(*this);
}

Expr * WhileStmt::getCond()
{
	return cond_.get();
}

Stmt * WhileStmt::getBody()
{
	return body_.get();
}

void WhileStmt::setCond(std::unique_ptr<Expr> cond)
{
	cond_ = std::move(cond);
}

void WhileStmt::setBody(std::unique_ptr<Stmt> body)
{
	body_ = std::move(body);
}

// DeclStmt
DeclStmt::DeclStmt(std::unique_ptr<Decl> decl) : decl_(std::move(decl)), Stmt(StmtKind::DeclStmt)
{

}

bool DeclStmt::hasDecl() const
{
	return (bool)decl_;
}

Decl * DeclStmt::getDecl()
{
	return decl_.get();
}

void DeclStmt::setDecl(std::unique_ptr<Decl> decl)
{
	decl_ = std::move(decl);
}

void DeclStmt::accept(IVisitor &vis)
{
	vis.visit(*this);
}