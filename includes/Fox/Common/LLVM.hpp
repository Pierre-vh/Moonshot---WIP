//----------------------------------------------------------------------------//
// This file is part of the Fox project.        
// See the LICENSE.txt file at the root of the project for license information.            
// File : LLVM.hpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
// This file imports the most commonly used LLVM functions that
// we want to use unqualified.
//----------------------------------------------------------------------------//

// We need to import some classes because they can't be easily forward
// declared
#include "llvm/Support/Casting.h" // reason: complex templates
#include "llvm/ADT/None.h"        // reason: can't fwd-decl without defining

// Forward-declare some llvm classes 
namespace llvm {
  template <typename T> class SmallVectorImpl;
  template <typename T, unsigned N> class SmallVector;
  template<typename T> class ArrayRef;
  template<typename T> class Optional;
  template<typename T> class MutableArrayRef;
}

namespace fox {
  using llvm::isa;
  using llvm::cast;
  using llvm::dyn_cast;
  using llvm::dyn_cast_or_null;
  using llvm::cast_or_null;

  using llvm::SmallVectorImpl;
  using llvm::SmallVector;

  using llvm::Optional;
  using llvm::None;

  using llvm::ArrayRef;
  using llvm::MutableArrayRef;
}