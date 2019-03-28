//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.     
// File : BCGenStmt.cpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//

#include "Fox/BCGen/BCGen.hpp"
#include "Registers.hpp"
#include "Fox/AST/Stmt.hpp"
#include "Fox/AST/ASTVisitor.hpp"
#include "Fox/BC/BCBuilder.hpp"
#include "Fox/Common/Errors.hpp"

using namespace fox;

//----------------------------------------------------------------------------//
// StmtGenerator 
//----------------------------------------------------------------------------//

class BCGen::StmtGenerator : public Generator,
                             StmtVisitor<StmtGenerator, void> {
  using Visitor = StmtVisitor<StmtGenerator, void>;
  friend Visitor;
  public:
    StmtGenerator(BCGen& gen, BCModuleBuilder& builder,
                  RegisterAllocator& regAlloc): 
      Generator(gen, builder), regAlloc(regAlloc), 
      theModule(builder.getModule()) {}

    void generate(Stmt* stmt) {
      visit(stmt);
    }

    RegisterAllocator& regAlloc;
    BCModule& theModule;

  private:
    // The type used to store jump offsets. Doesn't necessarily
    // match the one of the instructions.
    using jump_offset_t = std::int32_t;

    // The current maximum jump offset possible (positive or negative)
    // is the max (positive or negative) value of a 16 bit signed number:
    // 2^15-1
    static constexpr jump_offset_t max_jump_offset = (1 << 15)-1;

    //------------------------------------------------------------------------//
    // "emit" and "gen" methods 
    // 
    // These methods perform some generalized tasks related to bytecode
    // emission
    //------------------------------------------------------------------------//

    void genNode(ASTNode node) {
      if(Decl* decl = node.dyn_cast<Decl*>())
        bcGen.genLocalDecl(builder, regAlloc, decl);
      else if(Expr* expr = node.dyn_cast<Expr*>()) 
        bcGen.genDiscardedExpr(builder, regAlloc, expr);
      else if(Stmt* stmt = node.dyn_cast<Stmt*>()) 
        visit(stmt);
      else 
        fox_unreachable("Unknown ASTNode kind");
    }

    // This calculates the offset needed to jump from 'a' to 'b' so
    // 'b' is the next instruction that will be executed.
    jump_offset_t calculateJumpOffset(BCModule::instr_iterator a,
                                      BCModule::instr_iterator b) {
      if(a == b) 
        return 0;

      bool isNegative = false;
      if (b < a) {
        std::swap(a, b);
        isNegative = true;
      }

      // Calculate the distance between the a and the b iterator
      std::size_t diff = distance(a, b);

      // TODO: Replace this assertion by a proper 'fatal' diagnostic explaining
      // the problem. Maybe pass a lambda 'onOutOfRange' as parameter to
      // this function and call onOutOfRange() + return 0; when we try to
      // jump too far.
      assert((diff <= static_cast<std::size_t>(max_jump_offset))
        && "Jump is out of range!");

      // Convert it to jump_offset_t now that we know that it's safe.
      auto offset = static_cast<jump_offset_t>(diff);

      assert((offset != 0) && "offset cannot be zero");

      // Return, applying the minus if needed
      return isNegative ? (-offset) : offset;
    }

    //------------------------------------------------------------------------//
    // "visit" methods 
    // 
    // Theses methods will perfom the actual tasks required to emit
    // the bytecode for a statement.
    //------------------------------------------------------------------------//

    void visitCompoundStmt(CompoundStmt* stmt) {
      // Just visit all the nodes
      for (ASTNode node : stmt->getNodes()) {
        genNode(node);
        if (Stmt* nodeAsStmt = node.dyn_cast<Stmt*>()) {
          // If this is a ReturnStmt, stop here so we don't emit
          // the code after it (since it's unreachable anyway).
          if(isa<ReturnStmt>(nodeAsStmt)) return;
        }
      }
    }

    void visitConditionStmt(ConditionStmt* stmt) {
      // Gen the condition 
      RegisterValue condReg = bcGen.genExpr(builder, regAlloc, stmt->getCond());

      // Create a conditional jump (so we can skip the jump to the else's code 
      // when the condition is met)
      auto condJump = builder.createJumpIfInstr(condReg.getAddress(), 1);

      // Free the register of the condition
      condReg.free();

      // Now, create a jump to the else's code. It'll be completed later.
      auto jumpIfNot = builder.createJumpInstr(0);

      // Gen the 'then'
      visit(stmt->getThen());

      bool isThenEmpty = builder.isLastInstr(jumpIfNot);
      // If the 'then' is empty, remove the 'jumpIfNot'
      if (isThenEmpty) 
        builder.popInstr();

      // Compile the 'else' if present
      if (Stmt* elseBody = stmt->getElse()) {
        bool isElseEmpty = false;
        if(isThenEmpty) {
          // Since 'jumpIfNot' has been removed, condJump should
          // be the b instruction we have emitted.
          assert(builder.isLastInstr(condJump));
          // Gen the 'else'
          visit(elseBody);
          // Check if we have generated something. If yes, ajust the CondJump
          // to skip the else's code.
          isElseEmpty = builder.isLastInstr(condJump);
          auto off = calculateJumpOffset(condJump, 
                                         theModule.instrs_last());
          condJump->JumpIf.offset = off;
        }
        else {
          // Create a jump to the end of the condition so the then's code
          // skips the else's code.
          auto jumpEnd = builder.createJumpInstr(0);
          // Now we can complete 'jumpIfNot' so it executes the else.
          jumpIfNot->Jump.offset = calculateJumpOffset(jumpIfNot, jumpEnd);
          // Gen the 'else'
          visit(elseBody);
          // Check if we have generated something.
          isElseEmpty = builder.isLastInstr(jumpEnd);
          // complete 'jumpEnd' so it jumps to the b instruction emitted.
          jumpEnd->Jump.offset = calculateJumpOffset(jumpEnd, 
                                                     theModule.instrs_last());
        }

        // If both the 'then' and the 'else' were empty, remove everything after
        // (and including) the CondJump so only the condition's code is left.
        if (isThenEmpty && isElseEmpty)
          builder.truncate_instrs(condJump);
      }
      // No 'else' statement
      else {
        // If the 'then' was empty too, remove all of the code we've generated
        // related to the then/else, so only the condition's code is left.
        if (isThenEmpty) 
          builder.truncate_instrs(condJump);
        // Else, complete 'jumpToElse' to jump after the b instr emitted.
        else 
          jumpIfNot->Jump.offset = calculateJumpOffset(jumpIfNot, 
                                                       theModule.instrs_last());
      }
    }

    void visitWhileStmt(WhileStmt*) {
      fox_unimplemented_feature("WhileStmt BCGen");
      // Don't forget about variable liveliness: I'll need a
      // RegisterAllocator::LoopContext or something like that.
    }

    void visitReturnStmt(ReturnStmt*) {
      fox_unimplemented_feature("ReturnStmt BCGen");
    }
};

//----------------------------------------------------------------------------//
// BCGen Entrypoints
//----------------------------------------------------------------------------//

void BCGen::genStmt(BCModuleBuilder& builder, 
                    RegisterAllocator& regAlloc, 
                    Stmt* stmt) {
  assert(stmt && "stmt is null");
  StmtGenerator(*this, builder, regAlloc).generate(stmt);
}