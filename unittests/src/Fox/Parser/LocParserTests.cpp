//----------------------------------------------------------------------------//
// This file is part of the Fox project.        
// See the LICENSE.txt file at the root of the project for license information.            
// File : LocParserTests.cpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
//  Tests for the accuracy of Locations/Ranges of nodes generated by the Parser.
//----------------------------------------------------------------------------//

#include "gtest/gtest.h"
#include "Fox/Lexer/Lexer.hpp"
#include "Fox/Parser/Parser.hpp"
#include "Fox/Common/SourceManager.hpp"
#include "Fox/Common/DiagnosticEngine.hpp"
#include "Fox/AST/ASTContext.hpp"
#include "Fox/AST/Identifier.hpp"
#include "Support/TestUtils.hpp"

using namespace fox;

// Parser Preparator for LocTests
class LocTests : public ::testing::Test {
  public:
    LocTests() : diags(srcMgr), astContext(srcMgr, diags) {}

  protected:
    using ::testing::Test::SetUp;
    virtual void SetUp(const std::string& filepath) {
      fullFilePath = test::getPath(filepath);
      auto result = srcMgr.readFile(fullFilePath);
      file = result.first;
      // If file couldn't be loaded, give us the reason
      if (!file) {
        FAIL() << "Couldn't load file \""<< filepath << "\" in memory."
          "\n\tReason: " + toString(result.second);
      }

      lexer = std::make_unique<Lexer>(astContext);
      lexer->lexFile(file);

      if (astContext.hadErrors()) {
        FAIL() << "Lexing Error";
      }

      theUnit = UnitDecl::create(astContext, Identifier(), FileID());

      parser = std::make_unique<Parser>(astContext, lexer->getTokenVector(), theUnit);
      ok = true;
    }

    std::string fullFilePath;
    
    bool ok = false;
    DiagnosticEngine diags;
    FileID file;
    SourceManager srcMgr;
    UnitDecl* theUnit = nullptr;
    ASTContext astContext;
    std::unique_ptr<Lexer> lexer;
    std::unique_ptr<Parser> parser;
};

TEST_F(LocTests, FuncAndArgDecl) {
  SetUp("parser/loc/functions.fox");
  ASSERT_TRUE(ok) << "Initialization failed";
  auto presult = parser->parseFuncDecl();
  ASSERT_TRUE(presult) << "parsing error";

  FuncDecl* func = presult.castTo<FuncDecl>();
  auto funcRange = func->getRange();
  // First, test the function itself
  CompleteLoc func_beg = srcMgr.getCompleteLoc(funcRange.getBegin());
  CompleteLoc func_end = srcMgr.getCompleteLoc(funcRange.getEnd());
  
  EXPECT_EQ(func_beg, CompleteLoc(fullFilePath, 1, 1));
  EXPECT_EQ(func_end, CompleteLoc(fullFilePath, 4, 3));

  // Now, test the parameters
  ASSERT_TRUE(func->hasParams());
  ParamList& params = *func->getParams();
  ASSERT_EQ(params.getNumParams(), 2);

  // Extract each arg individually
  ParamDecl* arg1 = params[0];
  ParamDecl* arg2 = params[1];

  // Check if the names are right
  EXPECT_EQ(arg1->getIdentifier().getStr(), "_bar1");
  EXPECT_EQ(arg2->getIdentifier().getStr(), "_bar2");

  // Extract Arg locs
  #define BEG_LOC(x) srcMgr.getCompleteLoc(x->getBegin())
  #define END_LOC(x) srcMgr.getCompleteLoc(x->getEnd())
  
  auto arg1_beg = BEG_LOC(arg1);
  auto arg1_end = END_LOC(arg1);

  auto arg2_beg = BEG_LOC(arg2);
  auto arg2_end = END_LOC(arg2);

  #undef BEG_LOC
  #undef END_LOC

  EXPECT_EQ(arg1_beg, CompleteLoc(fullFilePath,1,10));
  EXPECT_EQ(arg1_end, CompleteLoc(fullFilePath,1,30));

  EXPECT_EQ(arg2_beg, CompleteLoc(fullFilePath, 1, 33));
  EXPECT_EQ(arg2_end, CompleteLoc(fullFilePath, 1, 45));

  // Extract arg type ranges
  auto arg1_typeRange = arg1->getTypeLoc().getRange();
  auto arg2_typeRange = arg2->getTypeLoc().getRange();

  // Extract locs
  auto arg1_tr_beg = srcMgr.getCompleteLoc(arg1_typeRange.getBegin());
  auto arg2_tr_beg = srcMgr.getCompleteLoc(arg2_typeRange.getBegin());

  // Check
  EXPECT_EQ(arg1_typeRange.getEnd(), arg1->getRange().getEnd());
  EXPECT_EQ(arg2_typeRange.getEnd(), arg2->getRange().getEnd());

  EXPECT_EQ(arg1_tr_beg, CompleteLoc(fullFilePath, 1, 26));
  EXPECT_EQ(arg2_tr_beg, CompleteLoc(fullFilePath, 1, 40));
}

// VarDecl test
TEST_F(LocTests, VarDecls) {
  SetUp("parser/loc/vardecl.fox");
  ASSERT_TRUE(ok) << "Initialization failed";
  auto presult = parser->parseVarDecl();
  ASSERT_TRUE(presult) << "parsing error";

  auto var = presult.castTo<VarDecl>();
  CompleteLoc var_beg = srcMgr.getCompleteLoc(var->getBegin());
  CompleteLoc var_end = srcMgr.getCompleteLoc(var->getEnd());

  EXPECT_EQ(var_beg, CompleteLoc(fullFilePath,1,3));
  EXPECT_EQ(var_end, CompleteLoc(fullFilePath,1,22));

  CompleteLoc var_ty_beg = srcMgr.getCompleteLoc(var->getTypeLoc().getBegin());
  CompleteLoc var_ty_end = srcMgr.getCompleteLoc(var->getTypeLoc().getEnd());

  EXPECT_EQ(var_ty_beg, CompleteLoc(fullFilePath, 1, 11));
  EXPECT_EQ(var_ty_end, CompleteLoc(fullFilePath, 1, 15));

  auto range = var->getInitExpr()->getRange();
  CompleteLoc expr_beg = srcMgr.getCompleteLoc(range.getBegin());
  CompleteLoc expr_end = srcMgr.getCompleteLoc(range.getEnd());

  EXPECT_EQ(expr_beg, CompleteLoc(fullFilePath, 1, 19));
  EXPECT_EQ(expr_end, CompleteLoc(fullFilePath, 1, 21));
}