////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : LocParserTests.cpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
//	Tests for the accuracy of Locations/Ranges of nodes generated by the Parser.
////------------------------------------------------------////

#include "gtest/gtest.h"
#include "Support/TestUtils.hpp"
#include "Utils.hpp"

using namespace Moonshot;

// Parser Preparator for LocTests
class LocTests : public ::testing::Test
{
	protected:
		virtual void SetUp(const std::string& filepath)
		{
			sourceManager = &(context.sourceManager);

			fullFilePath = Tests::convertRelativeTestResPathToAbsolute(filepath);
			file = context.sourceManager.loadFromFile(fullFilePath);

			// If file couldn't be loaded, give us the reason
			if (!file)
			{
				FAIL() << "Couldn't load file \""<< filepath << "\" in memory.";
			}

			lexer = std::make_unique<Lexer>(context, astContext);
			lexer->lexFile(file);

			if (!context.isSafe())
			{
				FAIL() << "Lexing Error";
			}

			parser = std::make_unique<Parser>(context, astContext, lexer->getTokenVector(), &declRecorder);
		}

		std::string fullFilePath;

		FileID file;
		Context context;
		ASTContext astContext;
		DeclRecorder declRecorder;
		SourceManager *sourceManager = nullptr;
		std::unique_ptr<Lexer> lexer;
		std::unique_ptr<Parser> parser;
};

TEST_F(LocTests, FuncAndArgDecl)
{
	SetUp("parser/loc/functions.fox");
	auto presult = parser->parseFunctionDecl();

	ASSERT_TRUE(presult) << "parsing error";
	auto func = presult.moveAs<FunctionDecl>();

	// First, test the function itself
	CompleteLoc func_beg = sourceManager->getCompleteLocForSourceLoc(func->getBegLoc());
	CompleteLoc func_head_end = sourceManager->getCompleteLocForSourceLoc(func->getHeaderEndLoc());
	CompleteLoc func_end = sourceManager->getCompleteLocForSourceLoc(func->getEndLoc());

	// Expected locs
	CompleteLoc expected_func_beg(fullFilePath, 1, 1);
	CompleteLoc expected_func_head_end(fullFilePath, 1, 91);
	CompleteLoc expected_func_end(fullFilePath, 4, 5);
	
	EXPECT_EQ(func_beg, expected_func_beg);
	EXPECT_EQ(func_head_end, expected_func_head_end);
	EXPECT_EQ(func_end, expected_func_end);

	// Now, test the args
	// Arg count should be correct
	ASSERT_EQ(func->argsSize(), 4);

	// Extract each arg individually
	ArgDecl* arg1 = func->getArg(0);
	ArgDecl* arg2 = func->getArg(1);
	ArgDecl* arg3 = func->getArg(2);
	ArgDecl* arg4 = func->getArg(3);

	// Check if the names are right
	EXPECT_EQ(arg1->getIdentifier()->getStr(), "_bar1");
	EXPECT_EQ(arg2->getIdentifier()->getStr(), "_bar2");
	EXPECT_EQ(arg3->getIdentifier()->getStr(), "_bar3");
	EXPECT_EQ(arg4->getIdentifier()->getStr(), "_bar4");

	// Extract all locs
	#define BEG_LOC(x) sourceManager->getCompleteLocForSourceLoc(x->getBegLoc())
	#define END_LOC(x) sourceManager->getCompleteLocForSourceLoc(x->getEndLoc())
	
	auto arg1_beg = BEG_LOC(arg1);
	auto arg1_end = END_LOC(arg1);

	auto arg2_beg = BEG_LOC(arg2);
	auto arg2_end = END_LOC(arg2);

	auto arg3_beg = BEG_LOC(arg3);
	auto arg3_end = END_LOC(arg3);

	auto arg4_beg = BEG_LOC(arg4);
	auto arg4_end = END_LOC(arg4);

	#undef BEG_LOC
	#undef END_LOC

	// Lines
	EXPECT_EQ(arg1_beg.line, 1);
	EXPECT_EQ(arg1_end.line, 1);

	EXPECT_EQ(arg2_beg.line, 1);
	EXPECT_EQ(arg2_end.line, 1);

	EXPECT_EQ(arg3_beg.line, 1);
	EXPECT_EQ(arg3_end.line, 1);

	EXPECT_EQ(arg4_beg.line, 1);
	EXPECT_EQ(arg4_end.line, 1);

	// Column
	EXPECT_EQ(arg1_beg.column, 10);
	EXPECT_EQ(arg1_end.column, 31);

	EXPECT_EQ(arg2_beg.column, 34);
	EXPECT_EQ(arg2_end.column, 50);

	EXPECT_EQ(arg3_beg.column, 53);
	EXPECT_EQ(arg3_end.column, 67);

	EXPECT_EQ(arg4_beg.column, 70);
	EXPECT_EQ(arg4_end.column, 82);
}

// VarDecl test
TEST_F(LocTests, VarDecls)
{
	SetUp("parser/loc/vardecl.fox");
	auto presult = parser->parseVarDecl();

	ASSERT_TRUE(presult) << "parsing error";
	auto var = presult.moveAs<VarDecl>();

	CompleteLoc var_beg = sourceManager->getCompleteLocForSourceLoc(var->getBegLoc());
	CompleteLoc var_end = sourceManager->getCompleteLocForSourceLoc(var->getEndLoc());

	EXPECT_EQ(var_beg.line, 1);
	EXPECT_EQ(var_end.line, 2);

	EXPECT_EQ(var_beg.column, 13);
	EXPECT_EQ(var_end.column, 8);
}