//----------------------------------------------------------------------------//
// Part of the Fox project, licensed under the MIT license.
// See LICENSE.txt in the project root for license information. 
// File : DiagnosticVerifier.hpp
// Author : Pierre van Houtryve
//----------------------------------------------------------------------------//
// This file declares the DiagnosticVerifier class.
// The DV offers a tools to parse "expect" instructions in a file, allowing
// the user to expect diagnostics and silence them.
// This tool is used to test the intepreter's diagnostics.
//
// An expect instruction's grammar is (roughly):
//    "expect-" <severity> ['@' ('+' | '-') <digit>] ':' <string>
// Examples:
//    expect-error@+1: foo is not bar
//    expect-note: foo declared here
//
//----------------------------------------------------------------------------//
// Feature Ideas:
//  - Allow raw line numbers as argument
//    e.g. expect-error@1: expected one or more declaration in unit
//
//  - Allow to expect a diagnostic multiple times
//    e.g. expect-error(3)@-1: 'foo' declared here
//----------------------------------------------------------------------------//

#pragma once

#include "SourceLoc.hpp"
#include "LLVM.hpp"
#include "string_view.hpp"
#include <set>
#include <map>

namespace fox {
  class DiagnosticEngine;
  class SourceManager;
  class Diagnostic;
  enum class DiagSeverity : std::uint8_t;
  class DiagnosticVerifier {
    using line_type = std::uint32_t;
    public:
      struct ExpectedDiag {
        ExpectedDiag() = default;
        ExpectedDiag(DiagSeverity sev, string_view str, FileID file, line_type line) :
          severity(sev), diagStr(str), file(file), line(line) {}

        DiagSeverity severity = DiagSeverity(0);
        string_view diagStr;
        FileID file;
        line_type line = 0;
        
        // For STL Containers
        bool operator<(const ExpectedDiag& other) const {
          return std::tie(severity, diagStr, file, line)
            < std::tie(other.severity, other.diagStr, other.file, other.line);
        }
      };

      using DiagsSetTy = std::multiset<ExpectedDiag>;

      DiagnosticVerifier(DiagnosticEngine& engine, SourceManager& srcMgr);

      /// Parses a file, searching for "expect-" directives, parsing them and 
      /// adding them to the list of expected diagnostics.
      /// \returns true if the file was parsed successfully, false otherwise.
      bool parseFile(FileID file);

      /// \returns the set of expected diagnostics
      DiagsSetTy& getExpectedDiags();

      /// Finishes verification.
      ///  If not all expected diagnostics were emitted, this will emit
      ///  an error + a unique note for each expected diagnostic that wasn't
      ///  emitted.
      ///
      /// \returns true if the verification is considered successful
      /// (all expected diags emitted), false otherwise.
      bool finish();

    protected:
      friend class DiagnosticEngine;

      /// Performs verification of a single diagnostic.
      ///  Returns false if the diagnostic shouldn't be consumed,
      ///  true if it can be consumed.
      bool verify(const Diagnostic& diag);

      /// Handles a verify instr, parsing it and processing it.
      /// \param loc the loc of the first char of the instr.
      /// \param instr the instruction string
      bool handleVerifyInstr(SourceLoc loc, string_view instr);

			/// Parses a verify instr, returning a ParsedInstr on success.
			Optional<ExpectedDiag> parseVerifyInstr(SourceLoc loc,
																								 string_view instr);

      void diagnoseZeroOffset(SourceLoc offsetDigitLoc);
			void diagnoseMissingStr(SourceLoc loc);
			void diagnoseMissingColon(SourceLoc loc);
      void diagnoseMissingSuffix(SourceLoc instrBeg);
			void diagnoseIllFormedOffset(SourceRange argRange);
      void diagnoseBadNegativeOffset(SourceRange argRange);

			/// Parses the suffix string and puts the result inside "expected"
			bool parseSeverity(string_view suffix, DiagSeverity& sev);

			/// Parses the offset string (e.g. "+3) and applies the offset
			bool parseOffset(SourceRange strRange,
										string_view str, std::int8_t& offset);

      bool hasEmittedUnexpectedDiagnostics_ = false;

      DiagnosticEngine& diags_;
      SourceManager& srcMgr_;
      DiagsSetTy expectedDiags_;
  };
} // namespace fox
