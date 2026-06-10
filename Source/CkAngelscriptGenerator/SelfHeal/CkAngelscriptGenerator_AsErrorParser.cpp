#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

#include "CkCore/Macros/CkMacros.h"

#include "Internationalization/Regex.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    auto
        FCk_AsParsedError::
        operator==(
            const FCk_AsParsedError& Other) const
        -> bool
    {
        return Kind              == Other.Kind
            && FilePath          == Other.FilePath
            && Line              == Other.Line
            && Column            == Other.Column
            && TargetNamespace   == Other.TargetNamespace
            && FunctionName      == Other.FunctionName
            && ArgsList          == Other.ArgsList
            && MissingIdentifier == Other.MissingIdentifier
            && LookupScope       == Other.LookupScope;
    }

    // ----------------------------------------------------------------------------------------------------------------

    namespace
    {
        // ---- Line classifiers ------------------------------------------------------

        // A standalone "<path>.as:" line establishes file context for subsequent
        // "(L:C) ..." error lines. Hazelight emits these as a banner before each
        // file's errors.
        auto Try_MatchFileHeader(
            const FString& InLine,
            FString&       OutFile) -> bool
        {
            const auto Trimmed = InLine.TrimStartAndEnd();
            if (NOT Trimmed.EndsWith(TEXT(".as:")))
            { return false; }

            OutFile = Trimmed.LeftChop(1);  // drop trailing ":"
            return true;
        }

        // "(L:C) No matching signatures to '<NS>::<func>(<args>)'"
        //
        // Captures the *last* "::" as the namespace/function separator: greedy
        // (.+) eats any nested namespace prefix, and the function-name class
        // [^':(]+ explicitly excludes ':' so the greedy match stops at the
        // correct boundary.
        //
        // Cascade filter: AS prints "Unknown" for any argument whose type
        // couldn't be resolved earlier in the same compile. Those errors are
        // downstream of a real root cause (typically a missing
        // FCk_Handle_<X> identifier the dispatcher's IdentifierNotADataType
        // path will pick up) — the cast/call site is symptomatic, not the
        // actual root. Filtering them at the parser keeps the dispatcher's
        // "Recognized N actionable roots" count honest and removes the
        // misleading "no strategy applies" terminal-banner warnings.
        auto Try_MatchNoMatchingSignatures(
            const FString&    InLine,
            const FString&    InCurrentFile,
            FCk_AsParsedError& OutError) -> bool
        {
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^\((\d+):(\d+)\) No matching signatures to '(.+)::([^':(]+)\(([^)]*)\)'$)")};

            auto Matcher = FRegexMatcher{Pattern, InLine.TrimStartAndEnd()};
            if (NOT Matcher.FindNext())
            { return false; }

            const auto ArgsList = Matcher.GetCaptureGroup(5);

            // Drop cascade-only matches — AS uses "Unknown" verbatim for
            // unresolved types. Real signatures never contain this token.
            if (ArgsList.Contains(TEXT("Unknown")))
            { return false; }

            OutError                  = FCk_AsParsedError{};
            OutError.Kind             = ECk_AsParsedError_Kind::NoMatchingSignatures;
            OutError.FilePath         = InCurrentFile;
            OutError.Line             = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column           = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            OutError.TargetNamespace  = Matcher.GetCaptureGroup(3);
            OutError.FunctionName     = Matcher.GetCaptureGroup(4);
            OutError.ArgsList         = ArgsList;
            return true;
        }

        // "(L:C) No matching signatures to '<Ident>(<args>)'" — no `::`.
        //
        // A constructor-style call on a type AS couldn't register. Observed
        // (2026-06 fresh-clone ESP boot test) when a caller directly
        // constructs a generated `F<X>_SpawnParams` whose canonical file is
        // missing: `auto P = FBb_X_SpawnParams();` errors with this form,
        // while the subsequent `P.Field = ...` lines cascade as
        // "'Field' is not a member of 'Unknown'" (dropped as noise — fixing
        // the root type fixes them).
        //
        // The identifier class [A-Za-z_][A-Za-z0-9_]* forbids ':' so this
        // can never shadow the namespace-qualified form above; matcher order
        // in ParseErrors is belt-and-braces only.
        auto Try_MatchBareCtorNoMatchingSignatures(
            const FString&    InLine,
            const FString&    InCurrentFile,
            FCk_AsParsedError& OutError) -> bool
        {
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^\((\d+):(\d+)\) No matching signatures to '([A-Za-z_][A-Za-z0-9_]*)\(([^)]*)\)'$)")};

            auto Matcher = FRegexMatcher{Pattern, InLine.TrimStartAndEnd()};
            if (NOT Matcher.FindNext())
            { return false; }

            const auto ArgsList = Matcher.GetCaptureGroup(4);

            // Same cascade filter as the namespace-qualified form.
            if (ArgsList.Contains(TEXT("Unknown")))
            { return false; }

            OutError                   = FCk_AsParsedError{};
            OutError.Kind              = ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures;
            OutError.FilePath          = InCurrentFile;
            OutError.Line              = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column            = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            OutError.MissingIdentifier = Matcher.GetCaptureGroup(3);
            OutError.ArgsList          = ArgsList;
            return true;
        }

        // "(L:C) Identifier '<X>' is not a data type [in namespace '<N>' or parent | in global namespace]"
        //
        // The in-clause is optional in the regex so we tolerate the (rare) case
        // where the error has no scope suffix. When present and naming a
        // namespace, group 4 captures the namespace name; "global namespace"
        // matches the second alternation and leaves group 4 empty.
        auto Try_MatchIdentifierNotADataType(
            const FString&    InLine,
            const FString&    InCurrentFile,
            FCk_AsParsedError& OutError) -> bool
        {
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^\((\d+):(\d+)\) Identifier '([^']+)' is not a data type(?: in namespace '([^']+)' or parent| in global namespace)?$)")};

            auto Matcher = FRegexMatcher{Pattern, InLine.TrimStartAndEnd()};
            if (NOT Matcher.FindNext())
            { return false; }

            OutError                   = FCk_AsParsedError{};
            OutError.Kind              = ECk_AsParsedError_Kind::IdentifierNotADataType;
            OutError.FilePath          = InCurrentFile;
            OutError.Line              = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column            = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            OutError.MissingIdentifier = Matcher.GetCaptureGroup(3);
            OutError.LookupScope       = Matcher.GetCaptureGroup(4);  // empty when "global namespace" or absent
            return true;
        }

        // "(L:C) Instead found '<string constant>'"
        //
        // Hazelight emits adjacent-string-literal errors as a TWO-line pair:
        //   (L:C) Expected ')' or ','
        //   (L:C) Instead found '<string constant>'
        // The first line's "Expected ')' or ','" is generic — it fires for many
        // parse errors (missing comma in arg list, unclosed paren, etc.). The
        // SECOND line's "Instead found '<string constant>'" is the unique
        // signature for the adjacent-literal splice case: AS expected a
        // separator and got the opening quote of a second literal.
        //
        // Matching only the second line keeps the regex unambiguous. Caller
        // source is left untouched (no auto-fix — that's user code, not
        // generated). The dispatcher classifies this as Author_FixupRequired_
        // AdjacentStringLiteral and logs an actionable "join the literals into
        // one string" banner instead of the generic "no recognized roots"
        // terminal that leaves headless test runs hanging without an
        // actionable diagnostic.
        auto Try_MatchAdjacentStringLiteral(
            const FString&    InLine,
            const FString&    InCurrentFile,
            FCk_AsParsedError& OutError) -> bool
        {
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^\((\d+):(\d+)\) Instead found '<string constant>'$)")};

            auto Matcher = FRegexMatcher{Pattern, InLine.TrimStartAndEnd()};
            if (NOT Matcher.FindNext())
            { return false; }

            OutError          = FCk_AsParsedError{};
            OutError.Kind     = ECk_AsParsedError_Kind::AdjacentStringLiteral;
            OutError.FilePath = InCurrentFile;
            OutError.Line     = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column   = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            return true;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsErrorParser::
        ParseErrors(
            const FString& InRawErrorOutput)
        -> TArray<FCk_AsParsedError>
    {
        auto Results    = TArray<FCk_AsParsedError>{};
        auto CurrentFile = FString{};

        auto Lines = TArray<FString>{};
        InRawErrorOutput.ParseIntoArrayLines(Lines, /*InCullEmpty=*/false);

        for (const auto& Line : Lines)
        {
            // File-context first — affects every subsequent "(L:C) ..." line.
            auto NewFile = FString{};
            if (Try_MatchFileHeader(Line, NewFile))
            {
                CurrentFile = NewFile;
                continue;
            }

            auto Error = FCk_AsParsedError{};
            if (Try_MatchNoMatchingSignatures(Line, CurrentFile, Error)
                || Try_MatchBareCtorNoMatchingSignatures(Line, CurrentFile, Error)
                || Try_MatchIdentifierNotADataType(Line, CurrentFile, Error)
                || Try_MatchAdjacentStringLiteral(Line, CurrentFile, Error))
            {
                Results.Add(MoveTemp(Error));
            }
            // Unrecognized lines (cascade noise, "Compiling ..." context, candidate
            // signature lists, blank lines, etc.) are intentionally dropped — see
            // header docstring for rationale.
        }

        return Results;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsErrorParser::
        DeduplicateRoots(
            const TArray<FCk_AsParsedError>& InErrors)
        -> TArray<FCk_AsParsedError>
    {
        auto Out  = TArray<FCk_AsParsedError>{};
        auto Seen = TSet<FString>{};

        for (const auto& Err : InErrors)
        {
            auto Key = FString{};
            switch (Err.Kind)
            {
                case ECk_AsParsedError_Kind::NoMatchingSignatures:
                    Key = FString::Printf(TEXT("nm|%s::%s(%s)"),
                        *Err.TargetNamespace, *Err.FunctionName, *Err.ArgsList);
                    break;
                case ECk_AsParsedError_Kind::IdentifierNotADataType:
                    Key = FString::Printf(TEXT("id|%s"), *Err.MissingIdentifier);
                    break;
                case ECk_AsParsedError_Kind::AdjacentStringLiteral:
                    // No qualified identifier — dedup by location instead. Two
                    // separate splices on different lines are distinct
                    // diagnostics; a re-emit of the same line is one.
                    Key = FString::Printf(TEXT("adj|%s:%d:%d"),
                        *Err.FilePath, Err.Line, Err.Column);
                    break;
                case ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures:
                    Key = FString::Printf(TEXT("ctor|%s(%s)"),
                        *Err.MissingIdentifier, *Err.ArgsList);
                    break;
            }

            if (Seen.Contains(Key))
            { continue; }
            Seen.Add(Key);
            Out.Add(Err);
        }

        return Out;
    }
}

// --------------------------------------------------------------------------------------------------------------------
