//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information.     
// File : Objects.hpp                      
// Author : Pierre van Houtryve                
//----------------------------------------------------------------------------//
// This file contains the Object hierarchy, which is used to represent
// Fox Objects such as Strings and Arrays.
//----------------------------------------------------------------------------//

#pragma once

#include "FoxTypes.hpp"
#include "string_view.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace fox {
  enum class ObjectKind : std::uint8_t {
    #define OBJECT(CLASS) CLASS,
    #include "Objects.def"
  };

  /// The base Object class
  class Object {
    public:
      ObjectKind getKind() const;

      // TODO: Allocation methods (custom operator new) ?
    protected:
      Object(ObjectKind kind);

    private:
      ObjectKind kind_;
  };

  /// StringObject is an immutable UTF8 String.
  ///   TODO: Trail-allocate the string once I get a proper allocator in
  ///         the VM.
  class StringObject : public Object {
    public:
      /// creates an empty StringObject
      StringObject();

      /// creates a StringObject from a pre-existing string_view
      StringObject(string_view value);

      /// \returns a const reference to the underlying string
      const std::string& str() const;

      /// \returns the size of the string in UTF8 codepoints.
      std::size_t length() const;
      /// \returns the size of the string in bytes
      std::size_t numBytes() const;

      FoxChar getChar(std::size_t n) const;

      static bool classof(const Object* obj) {
        return obj->getKind() == ObjectKind::StringObject;
      }

    private:
      const std::string str_;
  };

  /// ArrayObject is a dynamic, untyped array.
  class ArrayObject : public Object {
    public:
      /// The type of a single element in the Array.
      /// FIXME: This is some duplicated code with the VM's Register type.
      union Element {
        Element()                           : raw(0) {}
        explicit Element(std::uint64_t raw) : raw(raw) {}
        explicit Element(FoxInt v)          : intVal(v) {}
        explicit Element(FoxDouble v)       : doubleVal(v) {}
        explicit Element(bool v)            : boolVal(v) {}
        explicit Element(FoxChar v)         : charVal(v) {}
        explicit Element(Object* v)         : objectVal(v) {}

        /// the raw value of the element
        std::uint64_t raw;
        /// Integer elements
        FoxInt    intVal;
        /// Floating-point elements
        FoxDouble doubleVal;
        /// Boolean elements
        bool      boolVal;
        /// Char elements
        FoxChar   charVal;
        /// Object elements
        Object*   objectVal;
      };

      static_assert(sizeof(Element) == 8, 
        "Size of a single element is not 64 bits");

      using ArrayT = std::vector<Element>;

      /// Creates an empty array
      ArrayObject();

      /// Creates an array object, reserving \p n elements.
      /// NOTE: Elements are allocated, which means memory is allocated for them, but
      /// size() will still return 0 and attempting to retrieve this element will
      /// result in an out of range access.
      ArrayObject(std::size_t n);

      /// void push_back(Element elem);
      /// void set(std::size_t elem, Element)
      /// Element get(std::size_t elem);
      /// Element& operator[](std::size_t idx)
      /// const Element& operator[](std::size_t idx)

      /// std::size_t size()
      /// void reset()

      /// void pop_back();
      /// void erase(std::size_t start, std::size_t num);
      /// Element get(std::size_t elem);

      /// \returns a reference to the internal array of the ArrayObject
      ArrayT& data();
      /// \returns a const reference to the internal array of the ArrayObject
      const ArrayT& data() const;

    private:
      ArrayT data_;
  };
}