//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.     
// File : BCTests.cpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
//  Tests for Opcodes, Instructions, DebugInfo, BCModule, BCBuilder and 
//  BCFunction.
//----------------------------------------------------------------------------//

#include "gtest/gtest.h"
#include "Fox/BC/BCBuilder.hpp"
#include "Fox/BC/BCFunction.hpp"
#include "Fox/BC/BCModule.hpp"
#include "Fox/BC/BCUtils.hpp"
#include "Fox/BC/DebugInfo.hpp"
#include "Fox/BC/Instruction.hpp"
#include "Fox/Common/DiagnosticEngine.hpp"
#include "Fox/Common/FoxTypes.hpp"
#include "Fox/Common/LLVM.hpp"
#include "Fox/Common/SourceManager.hpp"
#include "llvm/ADT/ArrayRef.h"
#include <sstream>

using namespace fox;

//----------------------------------------------------------------------------//
// Opcode tests
//----------------------------------------------------------------------------//

TEST(OpcodeTest, opcodeToString) {
  Opcode a = Opcode::StoreSmallInt;
  Opcode b = Opcode::NoOp;
  Opcode c = Opcode::LAnd;

  const char* strA = to_string(a);
  const char* strB = to_string(b);
  const char* strC = to_string(c);

  ASSERT_NE(strA, nullptr);
  ASSERT_NE(strB, nullptr);
  ASSERT_NE(strC, nullptr);

  EXPECT_STRCASEEQ(strA, "StoreSmallInt");
  EXPECT_STRCASEEQ(strB, "NoOp");
  EXPECT_STRCASEEQ(strC, "LAnd");
}

//----------------------------------------------------------------------------//
// Instructions tests
//----------------------------------------------------------------------------//

TEST(InstructionTest, dumpInstructions) {
  InstructionVector instrs;
  BCBuilder builder(instrs);
  builder.createNoOpInstr();
  builder.createAddIntInstr(0, 1, 2);
  builder.createLNotInstr(42, 84);
  builder.createStoreSmallIntInstr(0, -4242);
  builder.createJumpInstr(-30000);;
  // Check that we have the correct number of instructions
  ASSERT_EQ(instrs.size(), 5u) << "Broken BCBuilder?";
  // Dump to a stringstream
  std::stringstream ss;
  dumpInstructions(ss, instrs);
  // Compare strings
  EXPECT_EQ(ss.str(),
    " 0\t| NoOp\n"
    " 1\t| AddInt 0 1 2\n"
    " 2\t| LNot 42 84\n"
    " 3\t| StoreSmallInt 0 -4242\n"
    " 4\t| Jump -30000\n");
}

TEST(InstructionTest, isAnyJump) {
  #define TEST_JUMP(OP)\
    EXPECT_TRUE(Instruction(Opcode::OP).isAnyJump())\
    << #OP " is a jump but isAnyJump returns false";
  #define TEST_NONJUMP(OP)\
    EXPECT_FALSE(Instruction(Opcode::OP).isAnyJump())\
    << #OP " isn't a jump but isAnyJump returns true";
  //---------------------------------------------------//
  TEST_JUMP(JumpIf);
  TEST_JUMP(JumpIfNot);
  TEST_JUMP(Jump);
  TEST_NONJUMP(StoreSmallInt);
  TEST_NONJUMP(AddInt);
  TEST_NONJUMP(LNot);
  //---------------------------------------------------//
  #undef TEST_JUMP
  #undef TEST_NONJUMP
}

//----------------------------------------------------------------------------//
// BCBuilder tests
//----------------------------------------------------------------------------//

TEST(BCBuilderTest, debugInfo) {
  using IRP = DebugInfo::IndexRangePair;

  SourceLoc aLoc(FileID(), 42);
  SourceLoc bLoc(FileID(), 84);
  
  SourceRange aRange(aLoc);
  SourceRange bRange(bLoc);

  InstructionVector instrs;
  DebugInfo dbg;
  BCBuilder builder(instrs, &dbg);


  // Create a few instruction and insert debug info for the 1st and 3rd one.
  // (So indexes 0 and 2)
  auto a = builder.createRetVoidInstr();
  builder.createRetVoidInstr();
  auto b = builder.createRetVoidInstr();
  builder.createRetVoidInstr();

  builder.addDebugRange(a, aRange);
  builder.addDebugRange(b, bRange);

  // Check the ranges
  auto dbgRanges = dbg.getRanges();
  ASSERT_EQ(dbgRanges.size(), 2u);
  EXPECT_EQ(dbgRanges[0], IRP(0, aRange));
  EXPECT_EQ(dbgRanges[1], IRP(2, bRange));

  // Check that we can fetch them
  EXPECT_EQ(dbg.getSourceRange(0), aRange);
  EXPECT_EQ(dbg.getSourceRange(2), bRange);
}

TEST(BCBuilderTest, TernaryInstr) {
  InstructionVector instrs;
  BCBuilder builder(instrs);
  // Create an Ternary instr
  auto it = builder.createAddIntInstr(42, 84, 126);
  // Check if it was encoded as expected.
  EXPECT_EQ(it->opcode, Opcode::AddInt);
  EXPECT_EQ(it->AddInt.dest, 42);
  EXPECT_EQ(it->AddInt.lhs, 84);
  EXPECT_EQ(it->AddInt.rhs, 126);
}

// Test for Binary Instrs with two 8 bit args.
TEST(BCBuilderTest, SmallBinaryInstr) {
  InstructionVector instrs;
  BCBuilder builder(instrs);
  // Create an Small Binary instr
  auto it = builder.createLNotInstr(42, 84);
  // Check if it was encoded as expected.
  EXPECT_EQ(it->opcode, Opcode::LNot);
  EXPECT_EQ(it->LNot.dest, 42);
  EXPECT_EQ(it->LNot.src, 84);
}

// Test for Binary Instrs with one 8 bit arg and one 16 bit arg.
TEST(BCBuilderTest, BinaryInstr) {
  InstructionVector instrs;
  BCBuilder builder(instrs);
  // Create a Binary instr
  auto it = builder.createStoreSmallIntInstr(42, 16000);
  // Check if was encoded as expected.
  EXPECT_EQ(it->opcode, Opcode::StoreSmallInt);
  EXPECT_EQ(it->StoreSmallInt.dest, 42);
  EXPECT_EQ(it->StoreSmallInt.value, 16000);
}

TEST(BCBuilderTest, UnaryInstr) {
  InstructionVector instrs;
  BCBuilder builder(instrs);
  // Create unary instrs (this one uses a signed value)
  auto positive = builder.createJumpInstr(30000);
  auto negative = builder.createJumpInstr(-30000);
  // Check the positive one
  {
    EXPECT_EQ(positive->opcode, Opcode::Jump);
    EXPECT_EQ(positive->Jump.offset, 30000);
  }
  // Check the negative one
  {
    EXPECT_EQ(negative->opcode, Opcode::Jump);
    EXPECT_EQ(negative->Jump.offset, -30000);
  }
}

TEST(BCBuilderTest, createdInstrIterators) {
  InstructionVector instrs;
  BCBuilder builder(instrs);
  // Create a few instructions, checking that iterators are valid
  auto a = builder.createJumpInstr(30000);
  EXPECT_EQ(a->opcode, Opcode::Jump);
  EXPECT_EQ(a->Jump.offset, 30000);
  auto b = builder.createJumpIfInstr(5, -4200);
  EXPECT_EQ(b->opcode, Opcode::JumpIf);
  EXPECT_EQ(b->JumpIf.condReg, 5u);
  EXPECT_EQ(b->JumpIf.offset, -4200);
  auto c = builder.createDivDoubleInstr(1, 2, 3);
  EXPECT_EQ(c->opcode, Opcode::DivDouble);
  EXPECT_EQ(c->DivDouble.dest, 1u);
  EXPECT_EQ(c->DivDouble.lhs, 2u);
  EXPECT_EQ(c->DivDouble.rhs, 3u);
  // Insert a few instructions, then tests the iterators again
  builder.createRetVoidInstr();
  builder.createNoOpInstr();
  auto last = builder.createRetVoidInstr();
  // a
  EXPECT_EQ(a->opcode, Opcode::Jump);
  EXPECT_EQ(a->Jump.offset, 30000);
  // b
  EXPECT_EQ(b->opcode, Opcode::JumpIf);
  EXPECT_EQ(b->JumpIf.condReg, 5u);
  EXPECT_EQ(b->JumpIf.offset, -4200);
  // c
  EXPECT_EQ(c->opcode, Opcode::DivDouble);
  EXPECT_EQ(c->DivDouble.dest, 1u);
  EXPECT_EQ(c->DivDouble.lhs, 2u);
  EXPECT_EQ(c->DivDouble.rhs, 3u);
  // ++c should be the break instr
  EXPECT_EQ((++c)->opcode, Opcode::RetVoid);
}

TEST(BCBuilderTest, orderIsRespected) {
  // Create some instructions in the builder
  InstructionVector instrs;
  BCBuilder builder(instrs);
  builder.createRetVoidInstr();
  builder.createNoOpInstr();
  builder.createAddIntInstr(0, 0, 0);
  builder.createAddDoubleInstr(0, 0, 0);
  // Create a vector of the expected opcodes
  SmallVector<Opcode, 4> expectedOps;
  expectedOps.push_back(Opcode::RetVoid);
  expectedOps.push_back(Opcode::NoOp);
  expectedOps.push_back(Opcode::AddInt);
  expectedOps.push_back(Opcode::AddDouble);
  // Check that the order matches what we expect
  {
    int idx = 0;
    for (auto it = instrs.begin(); it != instrs.end(); ++it)
      ASSERT_EQ(it->opcode, expectedOps[idx++]);
  }
}

//----------------------------------------------------------------------------//
// BCModule tests
//----------------------------------------------------------------------------//

namespace {
  class BCModuleTest : public ::testing::Test {
    public:
      SourceManager srcMgr;
      DiagnosticEngine diag = {srcMgr, std::cout};
      BCModule theModule = {srcMgr, diag};
  };
}

TEST_F(BCModuleTest, funcIDs) {
  // Create a few functions, checking that the IDs match the ones we expect.
  for (std::size_t idx = 0, end = 42; idx < end; ++idx) {
    ASSERT_EQ(theModule.createFunction().getID(), idx)
      << "Func ID was not the one expected";
  }
}

TEST_F(BCModuleTest, funcCreation) {
  BCFunction& fn = theModule.createFunction();
  fn.createBCBuilder().createNoOpInstr();
  EXPECT_EQ(&(theModule.getFunction(fn.getID())), &fn);
}

TEST_F(BCModuleTest, globIDs) {
  // Create a few functions, checking that the IDs match the ones we expect.
  for (std::size_t idx = 0, end = 42; idx < end; ++idx) {
    ASSERT_EQ(theModule.createGlobalVariable().getID(), idx)
      << "Func ID was not the one expected";
  }
}

TEST_F(BCModuleTest, globCreation) {
  BCFunction& fn = theModule.createGlobalVariable();
  fn.createBCBuilder().createNoOpInstr();
  EXPECT_EQ(&(theModule.getGlobalVarInitializer(fn.getID())), &fn);
}

TEST_F(BCModuleTest, newModulesAreEmpty) {
  ASSERT_TRUE(theModule.empty()) 
    << "newly created modules aren't considered empty";
}

TEST_F(BCModuleTest, dump) { 
  // Empty modules should display just "[Empty BCModule]" with a newline
  {
    std::stringstream ss;
    theModule.dump(ss);
    EXPECT_EQ(ss.str(), "[Empty BCModule]\n");
  }
  auto createFn = [&](bool isEntryPoint){
    BCFunction& func = theModule.createFunction();
    if(isEntryPoint)
      theModule.setEntryPoint(func);
    BCBuilder builder = func.createBCBuilder();
    builder.createRetVoidInstr();
    builder.createNoOpInstr();
    builder.createRetVoidInstr();
    builder.createLoadBuiltinFuncInstr(0, BuiltinKind::printBool);
  };
  // Add a function to trigger a more detailed dump
  createFn(false);
  {
    // Check the dump
    std::stringstream ss;
    theModule.dump(ss);
    EXPECT_EQ(ss.str(),
      "[No Constants]\n"
      "[No Globals]\n"
      "[Functions: 1][Entry Point: None]\n"
      "\n"
      "Function 0\n"
      "    0\t| RetVoid\n"
      "    1\t| NoOp\n"
      "    2\t| RetVoid\n"
      "    3\t| LoadBuiltinFunc 0 printBool\n"
    );
  }
  // Create a few more functions
  createFn(true);
  createFn(false);
  // Add a few constants
  theModule.addIntConstant(42);
  theModule.addIntConstant(0);
  theModule.addIntConstant(-42);
  theModule.addDoubleConstant(3.14);
  theModule.addDoubleConstant(-3.14);
  // Add a few globals
  theModule.createGlobalVariable().createBCBuilder().createNoOpInstr();
  theModule.createGlobalVariable().createBCBuilder().createNoOpInstr();
  theModule.createGlobalVariable().createBCBuilder().createNoOpInstr();
  // For strings, use special characters to see if they're correctly displayed
  theModule.addStringConstant("foobar");
  // The string must be constructed with an explicit size 
  // because it contains a '\0'
  theModule.addStringConstant(std::string("\n\t\r\\'\"\0", 7));
  {
    // Check the final dump
    std::stringstream ss;
    theModule.dump(ss);
    EXPECT_EQ(ss.str(),
      "[Constants]\n"
      "  [Integers: 3 constants]\n"
      "    0\t| 42\n"
      "    1\t| 0\n"
      "    2\t| -42\n"
      "  [Floating-Point: 2 constants]\n"
      "    0\t| 3.14\n"
      "    1\t| -3.14\n"
      "  [Strings: 2 constants]\n"
      "    0\t| \"foobar\"\n"
      "    1\t| \"\\n\\t\\r\\\\'\\\"\\0\"\n"
      "[Globals: 3]\n"
      "Initializer of Global 0\n"
      "    0\t| NoOp\n"
      "Initializer of Global 1\n"
      "    0\t| NoOp\n"
      "Initializer of Global 2\n"
      "    0\t| NoOp\n"
      "[Functions: 3][Entry Point: Function #1]\n"
      "\n"
      "Function 0\n"
      "    0\t| RetVoid\n"
      "    1\t| NoOp\n"
      "    2\t| RetVoid\n"
      "    3\t| LoadBuiltinFunc 0 printBool\n"
      "\n"
      "Function 1\n"
      "    0\t| RetVoid\n"
      "    1\t| NoOp\n"
      "    2\t| RetVoid\n"
      "    3\t| LoadBuiltinFunc 0 printBool\n"
      "\n"
      "Function 2\n"
      "    0\t| RetVoid\n"
      "    1\t| NoOp\n"
      "    2\t| RetVoid\n"
      "    3\t| LoadBuiltinFunc 0 printBool\n"
    );
  }
}

//----------------------------------------------------------------------------//
// BCFunction tests
//----------------------------------------------------------------------------//

TEST(BCFunctionTest, id) {
  BCFunction foo(42);
  EXPECT_EQ(foo.getID(), 42u);
}

TEST(BCFunctionTest, builder) {
  BCFunction fn(42);
  auto builder = fn.createBCBuilder();
  // Create some instrs
  builder.createNoOpInstr();
  builder.createRetVoidInstr();
  builder.createJumpIfInstr(0, 0);
  // Check that they're in the buffer
  SmallVector<Opcode, 4> expected = 
    { Opcode::NoOp, Opcode::RetVoid, Opcode::JumpIf};
  ASSERT_EQ(fn.numInstructions(), expected.size());
  for (std::size_t idx = 0, size = expected.size(); idx < size; ++idx)
    EXPECT_EQ(fn.getInstructions()[idx].opcode, expected[idx]);
}

TEST(BCFunctionTest, dump) {
  BCFunction fn(42);
  auto builder = fn.createBCBuilder();
  // Create some instrs
  builder.createNoOpInstr();
  builder.createRetVoidInstr();
  builder.createJumpIfInstr(1, 2);
  {
    std::stringstream ss;
    fn.dump(ss);
    EXPECT_EQ(ss.str(),
      "Function 42\n"
      "    0\t| NoOp\n"
      "    1\t| RetVoid\n"
      "    2\t| JumpIf 1 2\n");
  }
  {
    std::stringstream ss;
    fn.dump(ss, "Foo");
    EXPECT_EQ(ss.str(),
      "Foo 42\n"
      "    0\t| NoOp\n"
      "    1\t| RetVoid\n"
      "    2\t| JumpIf 1 2\n");
  }
}

//----------------------------------------------------------------------------//
// DebugInfo tests
//----------------------------------------------------------------------------//

TEST(DebugInfoTest, debugInfoTest) {
  SourceLoc aLoc(FileID(), 42);
  SourceLoc bLoc(FileID(), 84);
  SourceLoc cLoc(FileID(), 126);
  
  SourceRange aRange(aLoc);
  SourceRange bRange(bLoc);
  SourceRange cRange(cLoc);

  std::size_t aIdx = 42;
  std::size_t bIdx = 43;
  std::size_t cIdx = 4224;

  DebugInfo dbg;

  // Insert a few things
  dbg.addSourceRange(aIdx, aRange);
  dbg.addSourceRange(bIdx, bRange);
  dbg.addSourceRange(cIdx, cRange);

  // Check that the vector is sorted
  auto vec = dbg.getRanges();
  ASSERT_TRUE(
    std::is_sorted(
      vec.begin(), 
      vec.end(), 
      DebugInfo::IndexRangePairLessThanComparator())
  ) << "Vector of Ranges not Sorted!";

  // Try to fetch the elements
  auto check = [&](std::size_t idx, SourceRange expected) {
    auto result = dbg.getSourceRange(idx);
    if(result.hasValue())
      return result.getValue() == expected;
    return false;
  };
}

