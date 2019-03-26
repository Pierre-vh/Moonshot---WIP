//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.     
// File : Lexer.hpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
// This file declares the lexer class, which performs lexical analysis.    
//----------------------------------------------------------------------------//

// Note: Most of the code here is complete spaghetti that I wrote very early
// in the project.
// It'll be completly rewritten in the future. Same for Token.hpp/.cpp

#pragma once

#include "Token.hpp"
#include "Fox/Common/StringManipulator.hpp"

namespace fox {
  class DiagnosticEngine;
  class SourceManager;
  class Lexer  {
    public:
      Lexer(ASTContext &astctxt);

      void lexFile(FileID file);
  
      TokenVector& getTokens();
      std::size_t numTokens() const;  

      ASTContext& ctxt;
      DiagnosticEngine& diagEngine;
      SourceManager& srcMgr;

    private:
      // Pushes the current token with the kind 'kind' and advances.
      template<typename Kind>
      void pushTok(Kind kind) {
        tokens_.push_back(Token(kind, getCurtokStringView(), getCurtokRange()));
        advance();
        resetToken();
      }

      // Pushes a token of kind 'kind' consisting of a single codepoint
      // (calls resetToken() + pushToken())
      template<typename Kind>
      void beginAndPushToken(Kind kind) {
        resetToken();
        pushTok(kind);
      }

      // Calls advance(), then pushes the current token with the kind 'kind'
      template<typename Kind>
      void advanceAndPushTok(Kind kind) {
        advance();
        pushTok(kind);
      }

      // Returns true if we reached EOF.
      bool isEOF() const;

      // Begins a new token
      void resetToken();

      void lex();
      // Lexes an identifier or reserved keyword.
      void lexMaybeReservedIdentifier();
      void lexIntOrDoubleLiteral();
      bool lexCharItem();
      void lexCharLiteral();
      bool lexStringItem();
      void lexStringLiteral();
      void lexIntLiteral();

      // Handles a single-line comment
      void skipLineComment();
      // Handles a multi-line comment
      void skipBlockComment();

      // Returns true if the character is a <banned_text_item>
      bool isBannedTextItem(char c) const;

      // Returns true if 'ch' is a valid identifier head.
      bool isValidIdentifierHead(FoxChar ch) const;
      // Returns true if 'ch' is a valid identifier character.
      bool isValidIdentifierChar(FoxChar ch) const;

      // Returns the current character being considered
      FoxChar getCurChar() const;
      // Peeks the next character that will be considered
      FoxChar peekNextChar() const;

      // Advances to the next codepoint in the input.
      bool advance();

      SourceLoc getLocOfPtr(const char* ptr) const;
      SourceLoc getCurPtrLoc() const;
      SourceLoc getCurtokBegLoc() const;
      SourceRange getCurtokRange() const;
      string_view getCurtokStringView() const;

      FileID fileID_;

      const char* fileBeg_  = nullptr;
      const char* fileEnd_ = nullptr;
      // The pointer to the first byte of the token
      const char* tokBegPtr_  = nullptr;
      // The pointer to the current character being considered.
      // (pointer to the first byte in the UTF8 codepoint)
      const char* curPtr_  = nullptr;

      TokenVector tokens_;
  };
}
