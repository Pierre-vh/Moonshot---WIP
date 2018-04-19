////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : ParseDecl.cpp										
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
// This file implements decl, declstmt rules (methods)
// and related helper functions
////------------------------------------------------------////

#include "Parser.hpp"

#include "Moonshot/Fox/Basic/Context.hpp"
#include "Moonshot/Fox/AST/ASTContext.hpp"
#include "Moonshot/Fox/AST/ASTDecl.hpp"
#include "Moonshot/Fox/AST/ASTStmt.hpp"

#include <cassert>

using namespace Moonshot;

ParsingResult<ASTFunctionDecl*> Parser::parseFunctionDeclaration()
{
	/*
		<func_decl>		= "func" <id> '(' [<arg_decl> {',' <arg_decl>}*] ')'[':' <type>] <compound_statement>
		// Note about [':' <type>], if it isn't present, the function returns void
	*/

	// "func"
	if (matchKeyword(KeywordType::KW_FUNC))
	{
		auto rtr = std::make_unique<ASTFunctionDecl>();
		char ignoreNext = 0;
		// <id>
		if (auto id = matchID())
			rtr->setDeclName(id);
		else
		{
			errorExpected("Expected an identifier");
			return ParsingResult<ASTFunctionDecl*>(false);
		}

		// '('
		if (!matchSign(SignType::S_ROUND_OPEN))
		{
			errorExpected("Expected '('");
			// try to resync to a ) without consuming it.
			if (matchSign(SignType::S_COLON)) // if we got a colon 
			{
				// if the user typed "func foo : etc.. {", we recognize it and attempt to continue parsing.
				decrementPosition(); // Revert the token consume
				ignoreNext = 1; // Ignore the parenthesis
			}
			else if (matchSign(SignType::S_CURLY_OPEN))
			{
				// if the user typed "func foo {", we recognize it and attempt to continue parsing.
				decrementPosition(); // Revert the token consume
				ignoreNext = 2; // ignore parens + typespec
			}
			else
			{
				// try resync if possible
				if (!resyncToSignInFunction(SignType::S_ROUND_CLOSE, false))
					return ParsingResult<ASTFunctionDecl*>(false);
			}
		}

		// [<arg_decl> {',' <arg_decl>}*]
		if (auto firstarg_res = parseArgDecl())
		{
			// Note, here, in the 2 places I've marked with (1) and (2), we can possibly
			// add error management, however, I don't think that's necessary since
			// the matchSign below will attempt to "panic and recover" if it doesn't find the )
			rtr->addArg(std::move(firstarg_res.result));
			while (true)
			{
				if (matchSign(SignType::S_COMMA))
				{
					if (auto argdecl_res = parseArgDecl())
						rtr->addArg(std::move(argdecl_res.result));
					else
					{
						if (argdecl_res.wasSuccessful()) // not found?
							errorExpected("Expected an argument declaration");
						// (1)
					}
				}
				else
					break;
			}
		}
		// (2)

		// ')'
		if (!ignoreNext)
		{
			if (!matchSign(SignType::S_ROUND_CLOSE))
			{
				errorExpected("Expected a ')'");
				if (!resyncToSignInFunction(SignType::S_ROUND_CLOSE))
					return ParsingResult<ASTFunctionDecl*>(false);
			}
		}
		else
			ignoreNext--;
	
		// [':' <type>]
		if (!ignoreNext && matchSign(SignType::S_COLON))
		{
			auto rtrTy = parseType();
			if (rtrTy.first)
				rtr->setReturnType(rtrTy.first);
			else // no type found? we expected one after the colon!
			{
				if (rtrTy.second)
					errorExpected("Expected a type keyword");
				rtr->setReturnType(astcontext_.getPrimitiveVoidType());
				// don't return just yet, wait to see if a { can be found so we can still return something.
				// return ParsingResult<ASTFunctionDecl*>(false);
			}
		}
		else // if no return type, the function returns void.
		{
			if (!ignoreNext)
				ignoreNext--;

			rtr->setReturnType(astcontext_.getPrimitiveVoidType());
		}

		// Create recovery "enabling" object, since recovery is allowed for function bodies
		auto lock = createRecoveryEnabler();

		// <compound_statement>
		if (auto compstmt_res = parseCompoundStatement(/* mandatory = yes */ true))
		{
			rtr->setBody(std::move(compstmt_res.result));
			// Success, nothing more to see here!
			assert(rtr->isValid() && "Declaration is invalid but parsing function completed successfully?");
			return ParsingResult<ASTFunctionDecl*>(std::move(rtr));
		}
		else 
		{
			// Return an error if there was no compound statement.
			// We don't need to print an error, parseCompoundStatement will already have printed one
			// in mandatory mode.
			return ParsingResult<ASTFunctionDecl*>(false);
		}
	}
	// not found
	return ParsingResult<ASTFunctionDecl*>();
}

ParsingResult<ASTArgDecl*> Parser::parseArgDecl()
{
	// <arg_decl> = <id> <fq_type_spec>
	// <id>
	if (auto id = matchID())
	{
		// <fq_type_spec>
		if (auto typespec_res = parseFQTypeSpec())
			return ParsingResult<ASTArgDecl*>(
					std::make_unique<ASTArgDecl>(id,typespec_res.result)
				);
		else
		{
			if(typespec_res.wasSuccessful())		
				errorExpected("Expected a ':'");
			return ParsingResult<ASTArgDecl*>(false);
		}
	}
	return ParsingResult<ASTArgDecl*>();
}

ParsingResult<ASTVarDecl*> Parser::parseVarDeclStmt()
{
	// <var_decl> = "let" <id> <fq_type_spec> ['=' <expr>] ';'
	// "let"
	if (matchKeyword(KeywordType::KW_LET))
	{
		auto rtr = std::make_unique<ASTVarDecl>();

		// <id>
		if (auto id = matchID())
			rtr->setDeclName(id);
		else
		{
			errorExpected("Expected an identifier");
			if (auto res = resyncToSignInStatement(SignType::S_SEMICOLON))
			{
				if (res.hasRecoveredOnRequestedToken())
					return ParsingResult<ASTVarDecl*>(
							std::make_unique<ASTVarDecl>()	// If we recovered, return an empty (invalid) var decl.
						);
			}
			return ParsingResult<ASTVarDecl*>(false);
		}

		// <fq_type_spec>
		if (auto typespecResult = parseFQTypeSpec())
		{
			QualType ty = typespecResult.result;
			if (ty.isAReference())
			{
				context_.reportWarning("Ignored reference qualifier '&' in variable declaration : Variables cannot be references.");
				ty.setIsReference(false);
			}
			rtr->setType(ty);
		}
		else
		{
			errorExpected("Expected a ':'");
			if (auto res = resyncToSignInStatement(SignType::S_SEMICOLON))
			{
				if (res.hasRecoveredOnRequestedToken())
					return ParsingResult<ASTVarDecl*>(
							std::make_unique<ASTVarDecl>()	// If we recovered, return an empty (invalid) var decl.
						);
			}
			return ParsingResult<ASTVarDecl*>(false);
		}

		// ['=' <expr>]
		if (matchSign(SignType::S_EQUAL))
		{
			if (auto parseres = parseExpr())
				rtr->setInitExpr(std::move(parseres.result));
			else
			{
				if(parseres.wasSuccessful())
					errorExpected("Expected an expression");
				// Recover to semicolon, return if recovery wasn't successful 
				if (!resyncToSignInStatement(SignType::S_SEMICOLON, /* do not consume the semi, so it can be picked up below */false))
					return ParsingResult<ASTVarDecl*>(false);
			}
		}

		// ';'
		if (!matchSign(SignType::S_SEMICOLON))
		{
			errorExpected("Expected ';'");
			
			if(!resyncToSignInStatement(SignType::S_SEMICOLON))
				return ParsingResult<ASTVarDecl*>(false);
			// erecovery was successful, let the function return normally below.
		}
		// If we're here -> success
		assert(rtr->isValid() && "Declaration is invalid but parsing function completed successfully?");
		return ParsingResult<ASTVarDecl*>(std::move(rtr));
	}
	// not found
	return ParsingResult<ASTVarDecl*>();
}

ParsingResult<QualType> Parser::parseFQTypeSpec()
{
	// 	<fq_type_spec>	= ':' ["const"] ['&'] <type>
	if (matchSign(SignType::S_COLON))
	{
		QualType ty;
		// ["const"]
		if (matchKeyword(KeywordType::KW_CONST))
			ty.setConstAttribute(true);

		// ['&']
		if (matchSign(SignType::S_AMPERSAND))
			ty.setIsReference(true);

		// <type>
		auto type = parseType();
		// parseType returns a tuple of <const Type*, bool>. the first is the type, null on notfound,
		// the second is true on success, false on error.
		if (type.first)
			ty.setType(type.first);
		else
		{
			if(!type.second) 
				errorExpected("Expected a type");
			return ParsingResult<QualType>(false);
		}

		// Success!
		return ParsingResult<QualType>(ty);
	}
	// not found!
	return ParsingResult<QualType>();
}

ParsingResult<ASTDecl*> Parser::parseDecl()
{
	// <declaration> = <var_decl> | <func_decl>

	// <var_decl>
	if (auto vdecl = parseVarDeclStmt()) // we don't recover on error because recovery is handled by parseUnit.
		return ParsingResult<ASTDecl*>(std::move(vdecl.result));
	else if (!vdecl.wasSuccessful())
		return ParsingResult<ASTDecl*>(false);

	// <func_decl>
	if (auto fdecl = parseFunctionDeclaration())
		return ParsingResult<ASTDecl*>(std::move(fdecl.result));
	else if (!fdecl.wasSuccessful())
		return ParsingResult<ASTDecl*>(false);

	return ParsingResult<ASTDecl*>();
}