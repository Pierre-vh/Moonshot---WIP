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
//
//  TODO: Create proper interface/entry points for inserting a DiagnosticVerifier
//        in a given DiagEngine.
//----------------------------------------------------------------------------//

#pragma once

#include "Source.hpp"
#include "Diagnostic.hpp"
#include "DiagnosticConsumers.hpp"
#include <set>

namespace fox {
  class DiagnosticEngine;
	template<typename Ty> class ResultObject;
  class DiagnosticVerifier : public DiagnosticConsumer{
    using LineTy = CompleteLoc::LineTy;
    public:
      // Creates a DiagnosticVerifier. A DV will always require a consumer
      // attached to it, the value cannot be nullptr.
      DiagnosticVerifier(
        DiagnosticEngine& engine, 
        SourceManager& srcMgr, 
        std::unique_ptr<DiagnosticConsumer> consumer);

      // Parses a file, searching for "expect-" directives, parsing them and 
      // adding them to the list of expected diagnostics.
      // Returns true if the file was parsed successfully (no diags emitted),
      // false otherwise.
      bool parseFile(FileID file);

      virtual void consume(Diagnostic& diag) override;

      // Takes ownership of the consumer. Note that this class shouldn't be used
      // while there is no consumer, so be careful with that.
      std::unique_ptr<DiagnosticConsumer> takeConsumer();
      DiagnosticConsumer* getConsumer();
      const DiagnosticConsumer* getConsumer() const;
      // Sets the consumer. The old consumer will be destroyed in the operation.
      void setConsumer(std::unique_ptr<DiagnosticConsumer> consumer);

    private:
			struct ExpectedDiag {
        ExpectedDiag() = default;
				ExpectedDiag(DiagSeverity sev, string_view str, FileID file, LineTy line):
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
      // Handles a verify instr, parsing it and processing it.
      // The first argument is the loc of the first char of the instr.
      bool handleVerifyInstr(SourceLoc loc, string_view instr);

			// Parses a verify instr, returning a ParsedInstr on success.
			ResultObject<ExpectedDiag> parseVerifyInstr(SourceLoc loc,
																								 string_view instr);

			void diagnoseMissingStr(SourceLoc loc);
			void diagnoseMissingColon(SourceLoc loc);
      void diagnoseMissingSuffix(SourceLoc instrBeg);
			void diagnoseIllFormedOffset(SourceRange range);

			// Parses the suffix string and puts the result inside "expected"
			bool parseSeverity(string_view suffix, DiagSeverity& sev);

			// Parses the offset string (e.g. "+3) and applies the offset
			bool parseOffset(SourceRange strRange,
										string_view str, std::int8_t& offset);

      std::unique_ptr<DiagnosticConsumer> consumer_;
      DiagnosticEngine& diags_;
      SourceManager& srcMgr_;
      // Map of expected diagnostics
      std::multiset<ExpectedDiag> expectedDiags_;
  };
} // namespace fox