//----------------------------------------------------------------------------//
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : DiagnosticEngine.cpp											
// Author : Pierre van Houtryve								
//----------------------------------------------------------------------------//

#include "Fox/Common/DiagnosticEngine.hpp"
#include "Fox/Common/Source.hpp"
#include "Fox/Common/Errors.hpp"
#include <cassert>

using namespace fox;

static const char* diagsStrs[] = {
	#define DIAG(SEVERITY,ID,TEXT) TEXT,
		#include "Fox/Common/Diags/All.def"
};

static const DiagSeverity diagsSevs[] = {
	#define DIAG(SEVERITY,ID,TEXT) DiagSeverity::SEVERITY,
		#include "Fox/Common/Diags/All.def"
};

DiagnosticEngine::DiagnosticEngine(SourceManager& sm) : DiagnosticEngine(std::make_unique<StreamDiagConsumer>(sm))
{

}

DiagnosticEngine::DiagnosticEngine(std::unique_ptr<DiagnosticConsumer> ncons): consumer_(std::move(ncons))
{
	errLimitReached_ = false;
	hasFatalErrorOccured_ = false;
	errorsAreFatal_ = false;
	ignoreAll_ = false;
	ignoreAllAfterFatalError_ = false;
	ignoreNotes_ = false;
	ignoreWarnings_ = false;
	warningsAreErrors_ = false;
}

Diagnostic DiagnosticEngine::report(DiagID diagID)
{
	return report(diagID, SourceRange());
}

Diagnostic DiagnosticEngine::report(DiagID diagID, FileID file)
{
	return report(diagID, SourceRange(SourceLoc(file))).setIsFileWide(true);
}

Diagnostic DiagnosticEngine::report(DiagID diagID, SourceRange range)
{
	// Gather diagnostic data
	const auto idx = static_cast<typename std::underlying_type<DiagID>::type>(diagID);
	DiagSeverity sev = diagsSevs[idx];
	std::string str(diagsStrs[idx]);

	// Promote severity if needed
	sev = changeSeverityIfNeeded(sev);

	return Diagnostic(
				this,
				diagID,
				sev,
				str,
				range
			);;
}

Diagnostic DiagnosticEngine::report(DiagID diagID, SourceLoc loc)
{
	return report(diagID, SourceRange(loc));
}

void DiagnosticEngine::setConsumer(std::unique_ptr<DiagnosticConsumer> ncons)
{
	consumer_ = std::move(ncons);
}

DiagnosticConsumer* DiagnosticEngine::getConsumer()
{
	return consumer_.get();
}

bool DiagnosticEngine::hasFatalErrorOccured() const
{
	return hasFatalErrorOccured_;
}

std::uint16_t DiagnosticEngine::getWarningsCount() const
{
	return warnCount_;
}

std::uint16_t DiagnosticEngine::getErrorsCount() const
{
	return errorCount_;
}

std::uint16_t DiagnosticEngine::getErrorLimit() const
{
	return errLimit_;
}

void DiagnosticEngine::setErrorLimit(std::uint16_t mErr)
{
	errLimit_ = mErr;
}

bool DiagnosticEngine::getWarningsAreErrors() const
{
	return warningsAreErrors_;
}

void DiagnosticEngine::setWarningsAreErrors(bool val)
{
	warningsAreErrors_ = val;
}

bool DiagnosticEngine::getErrorsAreFatal() const
{
	return errorsAreFatal_;
}

void DiagnosticEngine::setErrorsAreFatal(bool val)
{
	errorsAreFatal_ = val;
}

bool DiagnosticEngine::getIgnoreWarnings() const
{
	return ignoreWarnings_;
}

void DiagnosticEngine::setIgnoreWarnings(bool val)
{
	ignoreWarnings_ = val;
}

bool DiagnosticEngine::getIgnoreNotes() const
{
	return ignoreNotes_;
}

void DiagnosticEngine::setIgnoreNotes(bool val)
{
	ignoreNotes_ = val;
}

bool DiagnosticEngine::getIgnoreAllAfterFatal() const
{
	return ignoreAllAfterFatalError_;
}

void DiagnosticEngine::setIgnoreAllAfterFatal(bool val)
{
	ignoreAllAfterFatalError_ = val;
}

bool DiagnosticEngine::getIgnoreAll() const
{
	return ignoreAll_;
}

void DiagnosticEngine::setIgnoreAll(bool val)
{
	ignoreAll_ = val;
}

void DiagnosticEngine::handleDiagnostic(Diagnostic& diag)
{
	if (diag.getDiagSeverity() != DiagSeverity::IGNORE)
	{
		assert(consumer_ && "No valid consumer");
		consumer_->consume(diag);
	}

	// Update our counters after consuming the diagnostic, because
	// some custom DiagnosticsConsumers might want to ignore specific
	// diagnostics.
	updateInternalCounters(diag.getDiagSeverity());

	// Now, check if we must emit a "too many errors" error.
	if ((errLimit_ != 0) && (errorCount_ >= errLimit_))
	{
		// If we should emit one, check if we haven't emitted one already.
		if (!errLimitReached_)
		{
			// Important : set this to true to avoid infinite recursion.
			errLimitReached_ = true;
			report(DiagID::diagengine_maxErrCountExceeded).addArg(errorCount_).emit();
			setIgnoreAll(true);
		}
	}
}

DiagSeverity DiagnosticEngine::changeSeverityIfNeeded(DiagSeverity ds) const
{
	using Sev = DiagSeverity;

	if (getIgnoreAll())
		return Sev::IGNORE;

	if (getIgnoreAllAfterFatal() && hasFatalErrorOccured())
		return Sev::IGNORE;

	switch (ds)
	{
		// Ignored diags don't change
		case Sev::IGNORE:
			return Sev::IGNORE;
		// Notes are silenced if the corresponding option is set
		case Sev::NOTE:
			return getIgnoreNotes() ? Sev::IGNORE : Sev::NOTE;
		// If Warnings must be silent, the warning is ignored.
		// Else, if the warnings are considered errors,
		// it is promoted to an error. If not, it stays a warning.
		case Sev::WARNING:
			if (getIgnoreWarnings())
				return Sev::IGNORE;
			return getWarningsAreErrors() ? Sev::ERROR : Sev::WARNING;
		// Errors are Ignored if too many of them have occured.
		// Else, it stays an error except if errors should be considered
		// Fatal.
		case Sev::ERROR:
			return getErrorsAreFatal() ? Sev::FATAL : Sev::ERROR;
		// Fatal diags don't change
		case Sev::FATAL:
			return ds;
		default:
			fox_unreachable("unknown severity");
	}
}

void DiagnosticEngine::updateInternalCounters(DiagSeverity ds)
{
	switch (ds)
	{
		case DiagSeverity::WARNING:
			warnCount_++;
			break;
		case DiagSeverity::ERROR:
			errorCount_++;
			break;
		case DiagSeverity::FATAL:
			hasFatalErrorOccured_ = true;
			break;
	}
}
