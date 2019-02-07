//----------------------------------------------------------------------------//
// This file is part of the Fox project.        
// See LICENSE.txt for license info.            
// File : ParseDecl.cpp                    
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//

#include "Fox/Parser/Parser.hpp"
#include "Fox/AST/ASTContext.hpp"
#include "Fox/AST/Types.hpp"
#include "Fox/Common/DiagnosticEngine.hpp"

using namespace fox;

UnitDecl* Parser::parseUnit(FileID fid, Identifier unitName) {
  // <fox_unit>  = {<declaration>}1+

  // Assert that unitName != nullptr
  assert(unitName && "Unit name cannot be nullptr!");
  assert(fid && "FileID cannot be invalid!");

  // Create the unit
  auto* unit = UnitDecl::create(ctxt, unitName, fid);

  // Create a RAIIDeclCtxt
  RAIIDeclCtxt raiidr(this, unit);

  bool declHadError = false;

  // Parse declarations 
  while (true) {
    if (auto parsedDecl = parseDecl()) {
      // We don't need to do anything, the Decl has been automatically
      // recorded because our DeclContext is the one that's currently active.
      continue;
    }
    else {
      if (parsedDecl.isError()) declHadError = true;

      // EOF/Died -> Break.
      if (isDone()) break;

      // No EOF? There's an unexpected token on the way that 
			// prevents us from finding the decl.
      else {
        // Report an error in case of "not found";
        if (parsedDecl.isNotFound()) {
          // Report the error with the current token being the error location
          Token curtok = getCurtok();
          assert(curtok 
            && "Curtok must be valid since we have not reached eof");
          diags.report(DiagID::parser_expected_decl, curtok.getRange());
        }

        if (resyncToNextDecl()) continue; 
        else break;
      }
    }
  }

  if (unit->numDecls() == 0) {
    if(!declHadError)
      diags.report(DiagID::parser_expected_decl_in_unit, fid);
    return nullptr;
  }
  else {
    actOnDecl(unit);
    return unit;
  }
}

Parser::Result<Decl*> Parser::parseFuncDecl() {
  /*
    <func_decl>  = "func" <id> '(' [<param_decl> {',' <param_decl>}*] ')
									 '[':' <type>] <compound_statement>
    // Note about [':' <type>], if it isn't present, the function returns void
  */

  // "func"
  auto fnKw = consumeKeyword(KeywordType::KW_FUNC);
  if (!fnKw) return Result<Decl*>::NotFound();
  assert(fnKw.getBegin() && "invalid loc info for func token");

  // Location information
  SourceLoc begLoc = fnKw.getBegin();

  // If invalid is set to true, it means that the declarations is missing
  // critical information and can't be considered as valid. If that's the case,
  // we won't return the declaration and we'll just return an error after
  // emitting all of our diagnostics.
  bool invalid = false;

  // <id>
  Identifier id;
  SourceRange idRange;
  {
    if (auto idRes = consumeIdentifier())
      std::tie(id, idRange) = idRes.getValue();
    else {
      reportErrorExpected(DiagID::parser_expected_iden);
      invalid = true;
    }
  }

  // '('
  if (!consumeBracket(SignType::S_ROUND_OPEN)) {
    if (invalid) return Result<Decl*>::Error();
    reportErrorExpected(DiagID::parser_expected_opening_roundbracket);
    return Result<Decl*>::Error();
  }

  // [<param_decl> {',' <param_decl>}*]
  ParamList* params = nullptr;
  {
    // Note: when parsing the ParamDecls, we don't want them to
    // be registered in any DeclContext, since we'll do it ourselves
    // later. To achieve that, we make the current decl context
    // a nullptr.
    RAIIDeclCtxt guard(this, nullptr);

    SmallVector<ParamDecl*, 4> paramsVec;
    if (auto first = parseParamDecl()) {
      paramsVec.push_back(first.castTo<ParamDecl>());
      while (true) {
        if (consumeSign(SignType::S_COMMA)) {
          if (auto param = parseParamDecl())
            paramsVec.push_back(param.castTo<ParamDecl>());
          else {
            // IDEA: Maybe reporting the error after the "," would yield
            // better error messages?
            if (param.isNotFound())
              reportErrorExpected(DiagID::parser_expected_paramdecl);
            return Result<Decl*>::Error();
          }
        } else break;
      }
    }
    // Stop parsing if the argument couldn't parse correctly.
    else if (first.isError()) return Result<Decl*>::Error();

    // Create the ParamList
    params = ParamList::create(ctxt, paramsVec);
    assert(params && "params are null!");
  }

  // ')'
  auto rightParens = consumeBracket(SignType::S_ROUND_CLOSE);
  if (!rightParens) {
    reportErrorExpected(DiagID::parser_expected_closing_roundbracket);

    // We'll attempt to recover to the '{' too,
		// so if we find the body of the function
    // we can at least parse that.
    if (!resyncToSign(SignType::S_ROUND_CLOSE, /*stop@semi*/ true, 
      /*consume*/ true))
      return Result<Decl*>::Error();
  }
  
  // [':' <type>]
  TypeLoc returnTypeLoc;
  if (auto colon = consumeSign(SignType::S_COLON)) {
    if (auto rtrTy = parseType())
      returnTypeLoc = rtrTy.get();
    else {
      if (rtrTy.isNotFound())
        reportErrorExpected(DiagID::parser_expected_type);

      if (!resyncToSign(SignType::S_CURLY_OPEN, true, false))
        return Result<Decl*>::Error();
    }
  }

  // If the function has no explicit return type, the return type is
  // void.
  if(!returnTypeLoc.isTypeValid())
    returnTypeLoc = TypeLoc(PrimitiveType::getVoid(ctxt), SourceRange());

  // Create the FuncDecl
  assert(id && idRange && params && returnTypeLoc.isTypeValid() &&
    "Can't create a FuncDecl with invalid data!");
  FuncDecl* func = FuncDecl::create(ctxt, getCurrentDeclCtxt(), begLoc,
    id, idRange, params, returnTypeLoc);

  {
    // Set this FuncDecl as the current DeclContext, so every decl
    // parsed within it's body has this function as parent.
    RAIIDeclCtxt guard(this, func);

    // <compound_statement>
    {
      if(Result<Stmt*> compStmt = parseCompoundStatement())
        func->setBody(cast<CompoundStmt>(compStmt.get()));
      else {
        if(compStmt.isNotFound()) // Display only if it was not found
          reportErrorExpected(DiagID::parser_expected_opening_curlybracket);
        return Result<Decl*>::Error();
      }
    }
  }

  // Finished parsing. If the decl is invalid, return an error.
  if (invalid) return Result<Decl*>::Error();

  // Record the FuncDecl
  actOnDecl(func);
  // Calculate it's ValueType.
  func->calculateValueType();
  assert(func->getValueType() && "FuncDecl type not calculated");
  return Result<Decl*>(func);
}

Parser::Result<Decl*> Parser::parseParamDecl() {
  // <param_decl> = <id> ':' ["mut"] <type>

  // <id>
  auto idRes = consumeIdentifier();
  if (!idRes) return Result<Decl*>::NotFound();
  Identifier id = idRes.getValue().first;
  SourceRange idRange = idRes.getValue().second;

  // ':'
  if (!consumeSign(SignType::S_COLON)) {
    reportErrorExpected(DiagID::parser_expected_colon);
    return Result<Decl*>::Error();
  }

  auto isMutable = (bool)consumeKeyword(KeywordType::KW_MUT);

  // <type>
  auto typeResult = parseType();
  if (!typeResult) {
    if (typeResult.isNotFound())
      reportErrorExpected(DiagID::parser_expected_type);
    return Result<Decl*>::Error();
  }

  TypeLoc tl = typeResult.get();

  assert(idRange && tl.getRange() && "Invalid loc info");

  auto* rtr = ParamDecl::create(ctxt, getCurrentDeclCtxt(), id, idRange, 
                                tl, isMutable); 
  actOnDecl(rtr);
  return Result<Decl*>(rtr);
}

Parser::Result<Decl*> Parser::parseVarDecl() {
  // <var_decl> = ("let" | "var") <id> ':' <type> ['=' <expr>] ';'

  // ("let" | "var")
  VarDecl::Keyword kw;
  SourceLoc begLoc;
  if (auto letKw = consumeKeyword(KeywordType::KW_LET)) {
    kw = VarDecl::Keyword::Let;
    begLoc = letKw.getBegin();
  } 
  else if(auto varKw = consumeKeyword(KeywordType::KW_VAR)) {
    kw = VarDecl::Keyword::Var;
    begLoc = varKw.getBegin();
  }
  else
    return Result<Decl*>::NotFound();
  
  // Helper lambda
  auto tryRecoveryToSemi = [&]() {
    if (resyncToSign(SignType::S_SEMICOLON, /*stop@semi*/false,
        /*consumeToken*/true)) {
      // If we recovered to a semicon, simply return not found.
      return Result<Decl*>::NotFound();
    }
    // Else, return an error.
    return Result<Decl*>::Error();
  };

  // <id>
  auto idRes = consumeIdentifier();
  if(!idRes) {
    reportErrorExpected(DiagID::parser_expected_iden);
    return tryRecoveryToSemi();
  }

  Identifier id = idRes.getValue().first;
  SourceRange idRange = idRes.getValue().second;

  // ':'
  if (!consumeSign(SignType::S_COLON)) {
    reportErrorExpected(DiagID::parser_expected_colon);
    return Result<Decl*>::Error();
  }

  // <type>
  TypeLoc type;
  if (auto typeRes = parseType())
    type = typeRes.get();
  else {
    if (typeRes.isNotFound())
      reportErrorExpected(DiagID::parser_expected_type);
    return tryRecoveryToSemi();
  }

  // ['=' <expr>]
  Expr* iExpr = nullptr;
  if (consumeSign(SignType::S_EQUAL)) {
    if (auto expr = parseExpr())
      iExpr = expr.get();
    else {
      if (expr.isNotFound())
        reportErrorExpected(DiagID::parser_expected_expr);
      // Recover to semicolon, return if recovery wasn't successful 
      if (!resyncToSign(SignType::S_SEMICOLON, 
				/*stop@semi*/ false, /*consumeToken*/ false))
        return Result<Decl*>::Error();
    }
  }

  // ';'
  SourceLoc endLoc = consumeSign(SignType::S_SEMICOLON);
  if (!endLoc) {
    reportErrorExpected(DiagID::parser_expected_semi);
      
    if (!resyncToSign(SignType::S_SEMICOLON, 
			/*stopAtSemi*/ false, /*consumeToken*/ false))
      return Result<Decl*>::Error();

    endLoc = consumeSign(SignType::S_SEMICOLON);
  }

  SourceRange range(begLoc, endLoc);
  assert(range && idRange && "Invalid loc info");
  assert(type.isComplete() && "Incomplete TypeLoc!");
  auto rtr = VarDecl::create(ctxt, getCurrentDeclCtxt(), id, idRange,
    type, kw, iExpr, range);

  actOnDecl(rtr);
  return Result<Decl*>(rtr);
}

Parser::Result<Decl*> Parser::parseDecl() {
  // <declaration> = <var_decl> | <func_decl>

  // <var_decl>
  if (auto vdecl = parseVarDecl())
    return vdecl;
  else if (vdecl.isError())
    return Result<Decl*>::Error();

  // <func_decl>
  if (auto fdecl = parseFuncDecl())
    return fdecl;
  else if (fdecl.isError())
    return Result<Decl*>::Error();

  return Result<Decl*>::NotFound();
}