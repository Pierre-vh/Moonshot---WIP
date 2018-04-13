////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Parser.hpp											
// Author : Pierre van Houtryve								
////------------------------------------------------------//// 
// This file implements the recursive descent parser.		
// The parser is implemented as a set of functions, each	
// function represents a rule in the grammar.				
// Some extra functions, for instance matchXXX	are used to help in the parsing process.					
//															
// The grammar used can be found in	/doc/grammar_(major).(minor).txt							
//															
// The lexer is the second step of the interpretation process:
// Lexer -> [PARSER] -> ...									
//															
// INPUT													
// It uses the data gathered and identified by the lexer to build a representation of the source file (AST.)		
//															
// OUTPUT													
// The Abstract Syntax Tree, AST for short.				
//
// Status: Up to date with latest grammar changes, but isn't finished yet.
////------------------------------------------------------////

#pragma once

#include "Moonshot/Fox/Lexer/Token.hpp"					
#include "Moonshot/Fox/Parser/ParsingResult.hpp"

#include "Moonshot/Fox/AST/ASTContext.hpp"
#include "Moonshot/Fox/AST/Types.hpp"
#include "Moonshot/Fox/AST/ASTDecl.hpp"
#include "Moonshot/Fox/AST/ASTExpr.hpp"
#include "Moonshot/Fox/AST/ASTStmt.hpp"
#include "Moonshot/Fox/AST/ASTUnit.hpp"

#include "Moonshot/Fox/AST/Operators.hpp"			

namespace Moonshot
{
	class Context;
	class Parser
	{
		public:
			Parser(Context& c,ASTContext* astctxt,TokenVector& l);

			// UNIT
			ParsingResult<ASTUnit*>	parseUnit();

			// EXPRESSIONS
			ParsingResult<IASTDeclRef*> parseArrayAccess(std::unique_ptr<IASTDeclRef> &base);
			ParsingResult<IASTDeclRef*> parseDeclCall(); 
			ParsingResult<ASTExpr*> parseLiteral();
			ParsingResult<ASTExpr*> parsePrimary();
			ParsingResult<ASTExpr*> parseMemberAccess();
			ParsingResult<ASTExpr*> parseExponentExpr();
			ParsingResult<ASTExpr*> parsePrefixExpr(); 
			ParsingResult<ASTExpr*> parseCastExpr();
			ParsingResult<ASTExpr*> parseBinaryExpr(const char &priority = 5);
			ParsingResult<ASTExpr*> parseExpr(); 

			// STATEMENTS
			ParsingResult<ASTStmt*> parseReturnStmt();
			ParsingResult<ASTStmt*> parseExprStmt(); // Expression statement
			ParsingResult<ASTCompoundStmt*> parseCompoundStatement(const bool& isMandatory=false, const bool& recoverOnError = false); 
			ParsingResult<ASTStmt*> parseStmt(); // General Statement
			ParsingResult<ASTStmt*> parseBody(); // body for control flow

			// STATEMENTS : CONDITION & LOOPS
			ParsingResult<ASTStmt*> parseCondition(); // Parse a  if-else if-else "block
			ParsingResult<ASTStmt*> parseWhileLoop();

			// DECLS
			ParsingResult<ASTVarDecl*> parseVarDeclStmt(const bool& recoverToSemiOnError = true);
			ParsingResult<ASTFunctionDecl*> parseFunctionDeclaration();
			ParsingResult<ASTDecl*> parseDecl();

		private:
			// expression helpers
			ParsingResult<ASTExpr*> parseParensExpr(const bool& isMandatory = false);
			ParsingResult<ExprList*> parseExprList();
			ParsingResult<ExprList*> parseParensExprList();
			// Arg decl & decl list
			ParsingResult<FunctionArg> parseArgDecl();
			ParsingResult<std::vector<FunctionArg>> parseArgDeclList();
			// Type spec
			ParsingResult<QualType> parseFQTypeSpec();

			// Type keyword
			// Returns a nullptr if no type keyword is found
			Type* parseTypeKw();

			// OneUpNode is a function that ups the node one level.
			// Example: There is a node N, with A and B as children. You call oneUpNode like this : oneUpNode(N,PLUS)
			// oneUpNode will return a new node X, with the operation PLUS and N as left child.
			std::unique_ptr<ASTBinaryExpr> oneUpNode(std::unique_ptr<ASTBinaryExpr> node, const binaryOperator &op = binaryOperator::DEFAULT);
			
			ParsingResult<LiteralInfo> matchLiteral();		// match a literal
			IdentifierInfo* matchID();						// match a ID. Returns the IdentifierInfo* if found, nullptr if not.
			bool matchSign(const SignType& s);				// match any signs : ; . ( ) , returns true if success
			bool matchKeyword(const KeywordType& k);		// match any keyword, returns true if success
			bool peekSign(const std::size_t &idx, const SignType &sign) const;
			
			bool matchExponentOp(); //  **
			ParsingResult<binaryOperator> matchAssignOp();						// = 
			ParsingResult<unaryOperator>  matchUnaryOp();						// ! - +
			ParsingResult<binaryOperator> matchBinaryOp(const char &priority);	// + - * / % 
			
			// GetToken
			Token getToken() const;
			Token getToken(const std::size_t &d) const;

			// Get state_.pos
			std::size_t getCurrentPosition() const;

			void incrementPosition();
			void decrementPosition();

			// This function will skip every token until the appropriate "resync" token is found. if consumeToken is set to false, the token won't be consumed.
			// Returns true if resync was successful.
			bool resyncToSign(const SignType &s,const bool& consumeToken = true);
			// Same as resyncToSign, except it works on "let" and "func" keywords
			bool resyncToNextDeclKeyword();
			// Helper for resyncToSign
			bool isClosingDelimiter(const SignType &s) const;			// Returns true if s is a } or ) or ]
			SignType getOppositeDelimiter(const SignType &s);			// Returns [ for ], { for }, ( for )

			// die : Indicates that the parsing is over and the parser has died because of a critical error. 
			void die();

			// Error helpers
			void errorUnexpected();
			void errorExpected(const std::string &s);
			void genericError(const std::string &s); 
			
			struct ParserState
			{
				std::size_t pos = 0;						// current pos in the Token vector.
				bool isAlive = true;						// is the parser "alive"?
			} state_;

			// Interrogate parser state
			// Returns true if pos >= tokens_.size()
			bool hasReachedEndOfTokenStream() const;
			bool isAlive() const;

			// Parser state backup
			ParserState createParserStateBackup() const;
			void restoreParserStateFromBackup(const ParserState& st);

			// Member variables
			ASTContext* astCtxt_ = nullptr;
			Context& context_;
			TokenVector& tokens_;	
	};
}