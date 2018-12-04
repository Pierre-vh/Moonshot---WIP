//----------------------------------------------------------------------------//
// This file is a part of The Moonshot Project.
// See the LICENSE.txt file at the root of the project for license information.
// File : DiagnosticVerifier.hpp
// Author : Pierre van Houtryve
//----------------------------------------------------------------------------//
// This file contains the DiagnosticVerifier diagnostic consumer class.
// The DV offers a tools to parse a file, finding every "expect-" instruction
// to silence expected diagnostics in tests, and doubles as a DiagnosticConsumer
// class which catches every diagnostic, checks if it was expected, and if
// that's the case, silences it.
//----------------------------------------------------------------------------//

#pragma once

#include "Source.hpp"
#include "Diagnostic.hpp"
#include "DiagnosticConsumers.hpp"
#include <set>

namespace fox {
  class DiagnosticEngine;
	template<typename Ty> class ResultObject;

  class DiagnosticVerifier {
    using LineTy = CompleteLoc::LineTy;
    public:
      struct ExpectedDiag {
        ExpectedDiag() = default;
        ExpectedDiag(DiagSeverity sev, string_view str, FileID file, LineTy line) :
          severity(sev), diagStr(str), file(file), line(line) {}

        DiagSeverity severity = DiagSeverity::IGNORE;
        string_view diagStr;
        FileID file;
        LineTy line = 0;
        
        // For STL Containers
        bool operator<(const ExpectedDiag& other) const {
          return std::tie(severity, diagStr, file, line)
            < std::tie(other.severity, other.diagStr, other.file, other.line);
        }
      };

      using DiagsSetTy = std::multiset<ExpectedDiag>;

      DiagnosticVerifier(DiagnosticEngine& engine, SourceManager& srcMgr);

      // Parses a file, searching for "expect-" directives, parsing them and 
      // adding them to the list of expected diagnostics.
      // Returns true if the file was parsed successfully (no diags emitted),
      // false otherwise.
      bool parseFile(FileID file);

      DiagsSetTy& getExpectedDiags();

      // Emits an error for each expected diagnostics that wasn't emitted.
      // Returns true if diagnostics were emitted, false otherwise.
      bool reportUnemittedDiags();

    protected:
      friend class DiagnosticEngine;

      // Performs verification of a single diagnostic
      // and returns true if we must emit the diagnostic, false otherwise.
      bool verify(Diagnostic& diag);

    private:
      // Handles a verify instr, parsing it and processing it.
      // The first argument is the loc of the first char of the instr.
      bool handleVerifyInstr(SourceLoc loc, string_view instr);

			// Parses a verify instr, returning a ParsedInstr on success.
			ResultObject<ExpectedDiag> parseVerifyInstr(SourceLoc loc,
																								 string_view instr);

      void diagnoseZeroOffset(SourceLoc offsetDigitLoc);
			void diagnoseMissingStr(SourceLoc loc);
			void diagnoseMissingColon(SourceLoc loc);
      void diagnoseMissingSuffix(SourceLoc instrBeg);
			void diagnoseIllFormedOffset(SourceRange range);

			// Parses the suffix string and puts the result inside "expected"
			bool parseSeverity(string_view suffix, DiagSeverity& sev);

			// Parses the offset string (e.g. "+3) and applies the offset
			bool parseOffset(SourceRange strRange,
										string_view str, std::int8_t& offset);

      DiagnosticEngine& diags_;
      SourceManager& srcMgr_;
      // Map of expected diagnostics
      DiagsSetTy expectedDiags_;
  };
} // namespace fox
