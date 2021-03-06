//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.      
// File : SemaDecl.cpp                    
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
//  This file implements Sema methods related to Decls and most of the 
//  Decl checking logic.
//----------------------------------------------------------------------------//

#include "Fox/Sema/Sema.hpp"
#include "Fox/AST/ASTContext.hpp"
#include "Fox/AST/ASTVisitor.hpp"
#include "Fox/AST/ASTWalker.hpp"
#include "Fox/AST/Types.hpp"
#include "Fox/Common/Errors.hpp"
#include "Fox/Common/DiagnosticEngine.hpp"
#include <map>

using namespace fox;

//----------------------------------------------------------------------------//
//  Sema::FuncFlowChecker
//----------------------------------------------------------------------------//

namespace {
  /// The "Function Flow Checker", which checks that a function body correctly
  /// returns on all control path, and warns about code after return statements.
  ///
  /// Every "visit" function return true if the node returns on all
  /// control paths, false otherwise.
  class FuncFlowChecker : StmtVisitor<FuncFlowChecker, bool> {
    using Inherited = StmtVisitor<FuncFlowChecker, bool>;
    friend Inherited;
    public:
      FuncFlowChecker(FuncDecl* decl, DiagnosticEngine& diagEngine) 
        : theFunc(decl), diagEngine(diagEngine) { }
        
      void check() {
        bool returnsOnAllPaths = visit(theFunc->getBody());
        if (!returnsOnAllPaths) {
          Type rtrTy = theFunc->getReturnTypeLoc().getType();
          // Function doesn't return void, and doesn't return
          // on all control paths:
          if (!rtrTy->isVoidType()) {
            // Diagnose at the } of the body
            SourceLoc loc = theFunc->getBody()->getEndLoc();
            Identifier ident = theFunc->getIdentifier();
            diagEngine.report(DiagID::missing_return_in_func, loc)
              .addArg(ident).addArg(rtrTy);
          }
        }
      }

      /// The function being checked
      FuncDecl* theFunc;
      /// The DiagnosticEngine used to report errors
      DiagnosticEngine& diagEngine;

    private:
      bool visitNode(ASTNode node) {
        // Only statements are interesting.
        if(Stmt* stmt = node.dyn_cast<Stmt*>())
          return visit(stmt);
        return false;
      }

      bool visitCompoundStmt(CompoundStmt* stmt) {
        bool returnsOnAllPaths = false;
        MutableArrayRef<ASTNode> nodes = stmt->getNodes();
        auto it = nodes.begin();
        auto end = nodes.end();
        for(; it != end; ++it) {
          // If we hit a node that returns on all control paths, set
          // returnsOnAllPaths to true and break the loop
          if (visitNode(*it)) {
            returnsOnAllPaths = true;
            break;
          }
        }
        // If the function returns on all control paths, check if there are
        // statements left in the CompoundStmt. 
        // If there are, warn about unreachable code.
        if (returnsOnAllPaths) {
          if ((++it) != end) {
            SourceLoc beg = it->getBeginLoc();
            diagEngine.report(DiagID::code_after_return_not_reachable, beg);
          }
        }
        return returnsOnAllPaths;
      }

      bool visitReturnStmt(ReturnStmt*) {
        // Obviously, a return statement always returns.
        return true;
      }

      bool visitWhileStmt(WhileStmt* stmt) {
        // Unfortunately, we can't do anything here, so just visit
        // the body and return false.
        visitCompoundStmt(stmt->getBody());
        // We might do something in the future when we can find infinite 
        // loops at compile time
        return false;
      }

      bool visitConditionStmt(ConditionStmt* stmt) {
        // For if-then-else statements, return true if both
        // return on all control paths.
        //
        // Else, just return "false" since we don't know if
        // the if's code will actually be executed.
        bool thenReturns = visitCompoundStmt(stmt->getThen());
        if (Stmt* elseStmt = stmt->getElse())
          return visit(elseStmt) && thenReturns;
        return false;
      }
  };
}

//----------------------------------------------------------------------------//
//  Sema::DeclChecker
//----------------------------------------------------------------------------//

class Sema::DeclChecker : Checker, DeclVisitor<DeclChecker, void> {
  using Inherited = DeclVisitor<DeclChecker, void>;
  friend Inherited;
  public:
    DeclChecker(Sema& sema) : Checker(sema) {}

    void check(Decl* decl) {
      assert(decl);
      assert(decl->isUnchecked() && "Decl has already been checked!");
      visit(decl);
    }

  private:
    //----------------------------------------------------------------------//
    // "diagnose" methods
    //----------------------------------------------------------------------//
    // Methods designed to diagnose specific situations.
    //----------------------------------------------------------------------//

    /// Diagnoses an illegal variable redeclaration. 
    /// "decl" is the illegal redecl, "decls" is the list of previous decls.
    ///
    /// If the decl shouldn't be considered a illegal redeclaration, returns
    /// false.
    bool diagnoseIllegalRedecl(NamedDecl* decl, const NamedDeclVec& decls) {
      // Find the original decl
      NamedDecl* earliest = findOriginalDecl(decl->getBeginLoc(), decls);
      // If there's a earliest decl, diagnose. 
      // (We might not have one if this is the first decl)
      if(earliest)
        diagnoseIllegalRedecl(earliest, decl);
      return (bool)earliest;
    }

    /// Helper for diagnoseIllegalRedecl
    DiagID getAppropriateDiagForRedecl(NamedDecl* original, NamedDecl* redecl) {
      // This is a "classic" var redeclaration
      if (isVarOrParamDecl(original) && isVarOrParamDecl(redecl)) {
        return isa<ParamDecl>(original) ?
          DiagID::invalid_param_redecl : DiagID::invalid_var_redecl;
      }
      // Invalid function redeclaration
      else if (isAnyFunc(original) && isAnyFunc(redecl))
        return DiagID::invalid_redecl;
      // Redecl as a different kind of symbol
      else
        return DiagID::invalid_redecl_diff_symbol_kind;
    }

    /// Diagnoses an illegal redeclaration where "redecl" is an illegal
    /// redeclaration of "original"
    void diagnoseIllegalRedecl(NamedDecl* original, NamedDecl* redecl) {
      // Quick checks
      Identifier id = original->getIdentifier();
      assert(original && redecl && "args cannot be null!");
      assert((id == redecl->getIdentifier())
        && "it's a redeclaration but names are different?");

      DiagID diagID = getAppropriateDiagForRedecl(original, redecl);
      diagEngine
        .report(diagID, redecl->getIdentifierRange())
        .addArg(id);
      noteFirstDeclHere(redecl->getFileID(), original);
    }

    /// Prints a "first decl seen here" type of note
    void noteFirstDeclHere(FileID inFile, NamedDecl* decl) {
      assert(inFile && "invalid FileID");
      if (isa<BuiltinFuncDecl>(decl)) {
        diagEngine.report(DiagID::is_a_builtin_func, inFile)
          .addArg(decl->getIdentifier());
      }
      else {
        diagEngine
          .report(DiagID::first_decl_seen_here, decl->getIdentifierRange())
          .addArg(decl->getIdentifier());
      }
    }

    /// Diagnoses an incompatible variable initializer.
    void diagnoseInvalidVarInitExpr(VarDecl* decl, Expr* init) {
      Type initType = init->getType();
      Type declType = decl->getValueType();

      if(!Sema::isWellFormed({initType, declType})) return;

      diagEngine
        .report(DiagID::invalid_vardecl_init_expr, init->getSourceRange())
        .addArg(initType)
        .addArg(declType)
        .setExtraRange(decl->getTypeLoc().getSourceRange());
    }

    //----------------------------------------------------------------------//
    // "visit" methods
    //----------------------------------------------------------------------//
    // Does semantic analysis on a single Decl
    //----------------------------------------------------------------------//

    void visit(Decl* decl) {
      decl->setCheckState(Decl::CheckState::Checking);
      Inherited::visit(decl);
      decl->setCheckState(Decl::CheckState::Checked);
    }

    void visitParamDecl(ParamDecl* decl) {
      checkForIllegalRedecl(decl);
    }

    void visitVarDecl(VarDecl* decl) {
      checkForIllegalRedecl(decl);

      if (Expr* init = decl->getInitExpr()) {
        bool ok = sema.typecheckExprOfType(init, decl->getValueType());
        decl->setInitExpr(init);
        if(!ok)
          diagnoseInvalidVarInitExpr(decl, init);
      }

      if(decl->hasInitExpr() && !decl->isLocal())
        checkGlobalVarInitExpr(decl->getInitExpr());
    }

    void visitFuncDecl(FuncDecl* decl) {
      auto raiiDC = sema.enterDeclCtxtRAII(decl);

      checkForIllegalRedecl(decl);

      // Check params
      if (ParamList* params = decl->getParams()) {
        for (ParamDecl* param : *params)
          visit(param);
      }

      // Check the body
      sema.checkStmt(decl->getBody());

      // Check the flow
      // NOTE: Ideally this check shouldn't run if the body of the func 
      // emitted errors. Unfortunately, there's no way to know that
      // for now.
      FuncFlowChecker(decl, diagEngine).check();

      // Check if this function can be our entry point.
      if (canBeEntryPoint(decl)) {
        // We shouldn't ever have duplicate entry points since duplicate
        // entry points will be illegal redeclarations that will be ignored.
        assert(!ctxt.getEntryPoint() && "entry point has already been set");
        ctxt.setEntryPoint(decl);
      }
    }

    void visitBuiltinFuncDecl(BuiltinFuncDecl*) {
      fox_unreachable("shouldn't be typechecked");
    }

    void visitUnitDecl(UnitDecl* unit) {
      auto dcGuard = sema.enterDeclCtxtRAII(unit);
      // Check every decl
      for(Decl* decl : unit->getDecls())
        visit(decl);
    }

    //----------------------------------------------------------------------//
    // Helper checking methods
    //----------------------------------------------------------------------//
    // Various semantics-related helper methods 
    //----------------------------------------------------------------------//

    /// Checks a global variable init expr. Such expression cannot contain
    /// references to other declarations.
    void checkGlobalVarInitExpr(Expr* expr) {
      class Impl : public ASTWalker {
        public:
          Impl(DiagnosticEngine& diagEngine) : diagEngine(diagEngine) {}
          
          DiagnosticEngine& diagEngine;

          std::pair<Expr*, bool> handleExprPre(Expr* expr) {
            if (isa<DeclRefExpr>(expr)) {
              diagEngine
                .report(DiagID::global_init_cannot_ref_other_decl, 
                        expr->getSourceRange());
              return {nullptr, false};
            }
            return {expr, true};
          }
      };
      Impl(diagEngine).walk(expr);
    }

    /// \returns true if \p decl can be the entry point of the program
    bool canBeEntryPoint(FuncDecl* decl) {
      // Cannot be an illegal redeclaration
      if(decl->isIllegalRedecl()) return false;
      // Decl must be in the "main" file
      if(decl->getFileID() != ctxt.getMainFileID()) return false;
      // Name must match the entry point's
      if(decl->getIdentifier() != ctxt.getEntryPointIdentifier()) return false;
      // Signature must match the entry point's
      return sema.unify(decl->getValueType(), ctxt.getEntryPointType());
    }

    /// Checks if "decl" is a illegal redeclaration.
    /// If "decl" is a illegal redecl, the appropriate diagnostic is emitted
    /// and this function returns false.
    /// Returns true if "decl" is legal redeclaration or not a redeclaration
    /// at all.
    void checkForIllegalRedecl(NamedDecl* decl) {        
      LookupOptions options;
      // If the Decl is local, only look in local decl contexts.
      options.onlyLookInLocalDeclContexts = decl->isLocal();
      options.shouldIgnore = [&](NamedDecl* result){

        // Ignore if result == decl
        if(result == decl) return true;
        // Ignore if result isn't from the same file
        if(result->getFileID() != decl->getFileID()) return true;
        return false;  // else, don't ignore.
      };

      Identifier id = decl->getIdentifier();
      LookupResult lookupResult;
      sema.doUnqualifiedLookup(lookupResult, id, decl->getBeginLoc(),
                               options);
      // If there are no matches, this cannot be a redecl
      if (lookupResult.size() == 0)
        return;
      else {
        // if we only have 1 result, and it's a ParamDecl
        NamedDecl* found = lookupResult.getIfSingleResult();
        if (found && isa<ParamDecl>(found) && !isa<ParamDecl>(decl)) {
          assert(decl->isLocal() && "Global declaration is conflicting with "
                 "a parameter declaration?");
          // Redeclaration of a ParamDecl by non ParamDecl is allowed
          // (a variable decl can shadow a parameter)
          return;
        }
        // Else, diagnose.
        bool isRedecl = diagnoseIllegalRedecl(decl, lookupResult.getDecls());
        // If it's indeed an illegal redeclaration, mark it.
        decl->setIsIllegalRedecl(isRedecl);
        return;
      }
    }

    //----------------------------------------------------------------------//
    // Other helper methods
    //----------------------------------------------------------------------//
    // Non semantics related helper methods
    //----------------------------------------------------------------------//
    
    /// Searches a lookup result to find the decl that should be considered
    /// the "original" decl when diagnosing for illegal redeclarations.
    ///
    /// The return result may be nullptr!
    NamedDecl* 
    findOriginalDecl(SourceLoc loc, const NamedDeclVec& decls) {
      assert(decls.size() && "Empty decl set");
      // First, add the decls to our own vector, but ignore:
      //  - unchecked decls
      //  - illegal redecls
      //  - decls that came after our decl
      SmallVector<NamedDecl*, 4> candidates; 
      candidates.reserve(decls.size());
      for (NamedDecl* decl : decls) {
        // Skip unchecked and illegal redeclarations
        if(decl->isUnchecked()) continue;
        if(decl->isIllegalRedecl()) continue;
        // Skip non-implicit declarations that were declared after
        // this position.
        if(!isa<BuiltinFuncDecl>(decl) 
          && !decl->getBeginLoc().comesBefore(loc))
          continue;
        candidates.push_back(decl);
      }

      // Maybe we already have our result
      if(candidates.size() == 0) return nullptr;
      if(candidates.size() == 1) return candidates[0];

      // Else, find the latest decl in the vector
      NamedDecl* candidate = nullptr;
      FileID file = loc.getFileID();
      for (NamedDecl* decl : candidates) {
        SourceLoc declLoc = decl->getBeginLoc();
        // If we have no candidate, take this decl as the first candidate
        if (!candidate) {
          candidate = decl;
          continue;
        }

        // If the current candidate is a Builtin, but this isn't a builtin,
        // this is automatically considered better.
        if (isa<BuiltinFuncDecl>(candidate) && !isa<BuiltinFuncDecl>(decl)) {
          candidate = decl;
          continue;
        }

        SourceLoc candLoc = candidate->getBeginLoc();
        // if this decl has been declared after the candidate, it
        // becomes the new candidate
        if (candLoc.comesBefore(declLoc))
          candidate = decl;
      }
      return candidate;
    }

    /// \returns true if decl is a VarDecl or ParamDecl
    bool isVarOrParamDecl(NamedDecl* decl) {
      return isa<ParamDecl>(decl) || isa<VarDecl>(decl);
    }

    /// \returns true if decl is a FuncDecl or BuiltinFuncDecl
    bool isAnyFunc(NamedDecl* decl) {
      return isa<BuiltinFuncDecl>(decl) || isa<FuncDecl>(decl);
    }

};

void Sema::checkUnitDecl(UnitDecl* decl) {
  checkDecl(decl);
}

void Sema::checkDecl(Decl* decl) {
  DeclChecker(*this).check(decl);
}