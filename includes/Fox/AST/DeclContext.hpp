//----------------------------------------------------------------------------//
// This file is a part of The Moonshot Project.        
// See the LICENSE.txt file at the root of the project for license information.            
// File : DeclContext.hpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
// Contains the DeclContext classes and related classes.
//----------------------------------------------------------------------------//

#pragma once

#include "Identifier.hpp"
#include "llvm/ADT/PointerIntPair.h"
#include "ASTAligns.hpp"
#include <map>
#include <vector>
#include <type_traits>

namespace fox {
  class Decl;
  class NamedDecl;
  class LookupResult;
  class FileID;

  // An iterator that abstracts the underlying structure 
  // (an iterator to std::pair) used by the DeclContext 
  // to only show the NamedDecl pointer to the client.
  template <typename BaseIterator>
  class DeclContextIterator : public BaseIterator {
    public:
      using value_type = typename BaseIterator::value_type::second_type;

      DeclContextIterator(const BaseIterator &baseIt) : BaseIterator(baseIt) {
        static_assert(std::is_same<value_type, NamedDecl*>::value,
					"Pointer type isn't a NamedDecl*");
      }

      // Operator * returns the pointer
      value_type operator*() const { 
        return (this->BaseIterator::operator*()).second; 
      }
      // Operator -> lets you access the members directly. It's equivalent to (*it)->
      value_type operator->() const { 
        return (this->BaseIterator::operator*()).second; 
      }
  };

  enum class DeclContextKind : std::uint8_t {
    #define DECL_CTXT(ID, PARENT) ID,
    #define LAST_DECL_CTXT(ID) LastDeclCtxt = ID
    #include "DeclNodes.def"
  };

  inline constexpr auto toInt(DeclContextKind kind) {
    return static_cast<std::underlying_type<DeclContextKind>::type>(kind);
  }

  // DeclContext is a class that acts as a "Declaration Recorder", which is
  // helps during semantic analysis. A DeclContext records every Declaration
  // that happens in it's children and has functions to help with Lookup. It
  // offers a Semantic view of the declarations it contains.
  //
  // It can also, on demand, generate a Lexical view of the Declarations
  // it contains.
  //
  // Keep in mind that, currently, the DeclContext has the following 
  // limitations.
  //  - It can only store non-null NamedDecl with a valid Identifier and
  //    valid source loc information.
  //  - It generates the lexical view on demand, each time it's
  //    requested. It is not cached.
  // Currently, Fox doesn't need such limitations to be lifted, as
  // everything stored within a DeclContext have been generated by the
  // Parser (= they are valid), and the Lexical View is currently
  // only used for Dumping the AST. It should be trivial enough
  // to cache the lexical view if needed though.
  class alignas(DeclContextAlignement) DeclContext {
    public:
      //----------------------------------------------------------------------//
      // Type Aliases
      //----------------------------------------------------------------------//

      // The type of the internal map of Decls
      using DeclsMapTy = std::multimap<Identifier, Decl*>;

      // The type of the map used to represent Decls in Lexical order.
      using LexicalDeclsTy = std::vector<Decl*>;

      // The type of the non-const iterator for DeclsMapTy
      using DeclMapIter 
        = DeclContextIterator<DeclsMapTy::iterator>;

      // The type of the const iterator for DeclsMapTy
      using DeclMapConstIter 
        = DeclContextIterator<DeclsMapTy::const_iterator>;

      //----------------------------------------------------------------------//
      // Interface
      //----------------------------------------------------------------------//

      // Constructor
      //  parent may be omitted
      DeclContext(DeclContextKind kind, DeclContext* parent = nullptr);

      // Returns the Kind of DeclContext this is
      DeclContextKind getDeclContextKind() const;

      // Adds a Decl in this DeclContext
      void addDecl(NamedDecl* decl);

      // Returns true if this is a local context
      bool isLocalDeclContext() const;

      // Getter for the DeclMap, which is a std::multimap. This map has
      // no particular ordering, and will be used for Lookup.
      const DeclsMapTy& getDeclsMap() const;

      // Returns a vector of all Stored Decl where the decls are
      // stored in order of appearance in the file "file"
      // Use this when you want to iterate over the decls in
      // Lexical order.
      // 
      // This is pretty costly to generate so only use this when absolutely
      // necessary.
      // (and it doesn't return a reference, so be careful about copies 
      //  of the return value)
      LexicalDeclsTy getLexicalDecls(FileID forFile);

      void setParent(DeclContext *dr);
      bool hasParent() const;
      DeclContext* getParent() const;

      // Get the number of decls in this DeclContext
      std::size_t numDecls()  const;

      static bool classof(const Decl* decl);

    private:
      //----------------------------------------------------------------------//
      // Private members
      //----------------------------------------------------------------------//

      // The PointerIntPair used to represent the ParentAndKind bits
      using ParentAndKindTy 
        = llvm::PointerIntPair<DeclContext*, DeclContextFreeLowBits>;
      // Members
      ParentAndKindTy parentAndKind_;
      DeclsMapTy decls_;

      //----------------------------------------------------------------------//
      // Static assertion
      //----------------------------------------------------------------------//
      // Check that ParentAndKindTy has enough bits to represent
      // every possible DeclContextKind
      static_assert(
        (1 << DeclContextFreeLowBits) > toInt(DeclContextKind::LastDeclCtxt),
        "The PointerIntPair doesn't have enough bits to represent every "
        " DeclContextKind value");
  };
}